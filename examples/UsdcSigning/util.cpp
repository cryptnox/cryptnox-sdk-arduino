/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

#include <Arduino.h>
#include "util.h"

/**
 * @brief Convert a hexadecimal character to a byte value.
 *
 * M-07: returns -1 on any non-hex character so callers can distinguish a
 * true '0' from a parse error.
 */
int fromHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/**
 * @brief Convert a hex string to a byte array.
 *
 * H-03: validate input length before any read. Without this guard a truncated
 * or malicious RPC response shorter than 2*len would cause out-of-bounds reads
 * past the NUL terminator.
 *
 * @return true on success, false on NULL input or insufficient length.
 */
// cppcheck-suppress unusedFunction
bool hexToBytes(const char* hex, uint8_t* out, size_t len) {
    if ((hex == nullptr) || (out == nullptr)) {
        return false;
    }
    if (strlen(hex) < (2U * len)) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        int hi = fromHex(hex[2*i]);
        int lo = fromHex(hex[2*i+1]);
        /* M-07: reject invalid characters instead of silently mapping them to 0. */
        if ((hi < 0) || (lo < 0)) {
            return false;
        }
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

/**
 * @brief Encodes the RLP list header for a sequence of items.
 *
 * This function generates the RLP "whole header" for a list payload of length total_len,
 * according to Ethereum RLP specification.
 *
 * RLP encoding rules:
 * - For a list with total payload < 55 bytes:
 *   0xC0 + total_len (single-byte header)
 * - For a list with payload >= 55 bytes:
 *   0xF7 + length_of_length_field, followed by the big-endian encoded total_len
 *
 * Example:
 * total_len = 10 → header = C0 + 0x0A → C0 0A
 * total_len = 100 → total_len = 0x64 (1 byte) → F8 64
 * total_len = 1024 → total_len = 0x0400 (2 bytes) → F9 04 00
 *
 * This header is intended to be placed immediately before the concatenated items
 * in an RLP list. For EIP-1559 typed transactions, it must appear after the type byte (0x02).
 *
 * @param[out] header_output Pointer where the encoded header will be written.
 * @param[in] total_len Length in bytes of all list items concatenated.
 * @return Number of bytes written to header_output.
 */
// cppcheck-suppress unusedFunction
uint32_t RlpEncodeWholeHeader(uint8_t* header_output, size_t header_cap, uint32_t total_len)
{
    if (total_len < 55U)
    {
        if (header_cap < 1U) { return 0U; }
        header_output[0] = (uint8_t)0xc0 + (uint8_t)total_len;
        return 1U;
    }
    else
    {
        uint8_t tmp_header[8];
        memset(tmp_header, 0, 8U);
        uint32_t hexdigit = 1U;
        uint32_t tmp = total_len;
        while ((uint32_t)(tmp / 256U) > 0U)
        {
            tmp_header[hexdigit] = (uint8_t)(tmp % 256U);
            tmp = (uint32_t)(tmp / 256U);
            hexdigit++;
        }
        tmp_header[hexdigit] = (uint8_t)(tmp);
        tmp_header[0] = (uint8_t)0xf7 + (uint8_t)hexdigit;

        // fix direction for header
        uint8_t header[8];
        memset(header, 0, 8U);
        header[0] = tmp_header[0];
        for (uint32_t i = 0U; i < hexdigit; i++)
        {
            header[i + 1U] = tmp_header[hexdigit - i];
        }
        const size_t need = (size_t)hexdigit + 1U;
        if (need > header_cap) { return 0U; }
        memcpy(header_output, header, need);
        return hexdigit + 1U;
    }
}

/**
 * @brief Encodes a single RLP item.
 *
 * @param[out] output Buffer where the encoded RLP item will be written.
 * @param[in] input Input data to encode.
 * @param[in] input_len Length of input data.
 * @return Number of bytes written to output.
 */
// cppcheck-suppress unusedFunction
uint32_t RlpEncodeItem(uint8_t* output, size_t output_cap, const uint8_t* input, uint32_t input_len)
{
    if (input_len == 1U && input[0] == 0x00U)
    {
        if (output_cap < 1U) { return 0U; }
        const uint8_t c[1] = {0x80};
        memcpy(output, c, 1U);
        return 1U;
    }
    else if (input_len == 1U && input[0] < 128U)
    {
        if (output_cap < 1U) { return 0U; }
        memcpy(output, input, 1U);
        return 1U;
    }
    else if (input_len <= 55U)
    {
        const size_t need = (size_t)input_len + 1U;
        if (need > output_cap) { return 0U; }
        const uint8_t _ = (uint8_t)0x80 + (uint8_t)input_len;
        const uint8_t header[] = {_};
        memcpy(output, header, 1U);
        memcpy(output + 1U, input, (size_t)input_len);
        return input_len + 1U;
    }
    else
    {
        uint8_t tmp_header[8];
        memset(tmp_header, 0, 8U);
        uint32_t hexdigit = 1U;
        uint32_t tmp = input_len;
        while ((uint32_t)(tmp / 256U) > 0U)
        {
            tmp_header[hexdigit] = (uint8_t)(tmp % 256U);
            tmp = (uint32_t)(tmp / 256U);
            hexdigit++;
        }
        tmp_header[hexdigit] = (uint8_t)(tmp);
        tmp_header[0] = (uint8_t)0xb7 + (uint8_t)hexdigit;

        // fix direction for header
        uint8_t header[8];
        memset(header, 0, 8U);
        header[0] = tmp_header[0];
        for (uint32_t i = 0U; i < hexdigit; i++)
        {
            header[i + 1U] = tmp_header[hexdigit - i];
        }
        const size_t need = (size_t)input_len + hexdigit + 1U;
        if (need > output_cap) { return 0U; }
        memcpy(output, header, hexdigit + 1U);
        memcpy(output + hexdigit + 1U, input, (size_t)input_len);
        return input_len + hexdigit + 1U;
    }
}

/**
 * @brief Converts an unsigned integer into a big-endian byte array.
 *
 * NEW-5: widened to uint64_t. Previously truncated Tx2 uint64 fields
 * (nonce/fees/gasLimit/value) at 4 bytes, producing wrong-amount tx.
 *
 * @param[out] str Output buffer (at least 8 bytes).
 * @param[in] val Unsigned 64-bit integer to convert.
 * @return Number of bytes written to the buffer (1..8).
 */
// cppcheck-suppress unusedFunction
uint32_t ConvertNumberToUintArray(uint8_t* str, uint64_t val)
{
    uint32_t ret = 0U;
    uint8_t tmp[8];
    memset(tmp, 0, 8U);

    while ((val / 256ULL) > 0ULL)
    {
        tmp[ret] = (uint8_t)(val % 256ULL);
        val = val / 256ULL;
        ret++;
    }
    tmp[ret] = (uint8_t)(val % 256ULL);
    for (uint32_t i = 0U; i < ret + 1U; i++)
    {
        str[i] = tmp[ret - i];
    }

    return ret + 1U;
}

/**
 * @brief Trims leading zeros from a byte array.
 *
 * @param[out] out Output buffer.
 * @param[in] in Input buffer.
 * @param[in] in_len Length of input buffer.
 * @return Number of bytes written to the output buffer.
 */
// cppcheck-suppress unusedFunction
size_t trimLeadingZeros(uint8_t* out, size_t out_cap, const uint8_t* in, size_t in_len)
{
    if ((in_len == 0U) || (out_cap == 0U)) { return 0U; }
    size_t start = 0;
    while (start < in_len - 1 && in[start] == 0)
    {
        start++;
    }
    size_t len = in_len - start;
    /* cppcheck-suppress knownConditionTrueFalse
     * cppcheck only follows the loop-exit path where start == in_len-1
     * (giving len == 1) and misses the early exit when in[start] != 0
     * (giving len up to in_len). The check is genuine defense-in-depth
     * if a future caller passes out_cap < in_len. */
    if (len > out_cap) { return 0U; }
    memcpy(out, in + start, len);
    return len;
}
