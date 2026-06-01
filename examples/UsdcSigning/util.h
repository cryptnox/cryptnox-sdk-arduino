/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Encodes the total length into an RLP list header.
 *
 * Bounds-checked: returns 0 (and does not write) if the encoded header
 * would not fit in @p header_cap bytes.
 *
 * @param[out] header_output Pointer to buffer to store the RLP header.
 * @param[in]  header_cap    Capacity of @p header_output in bytes.
 * @param[in]  total_len     Total length of the RLP list content.
 * @return Number of bytes written into header_output, or 0 on overflow.
 */
uint32_t RlpEncodeWholeHeader(uint8_t *header_output, size_t header_cap, uint32_t total_len);

/**
 * @brief Encodes a single item in RLP format.
 *
 * Bounds-checked: returns 0 (and does not write) if the encoded item
 * would not fit in @p output_cap bytes.
 *
 * @param[out] output     Pointer to buffer to store the RLP-encoded item.
 * @param[in]  output_cap Capacity of @p output in bytes.
 * @param[in]  input      Pointer to the data to encode.
 * @param[in]  input_len  Length of the input data in bytes.
 * @return Number of bytes written into output, or 0 on overflow.
 */
uint32_t RlpEncodeItem(uint8_t* output, size_t output_cap, const uint8_t* input, uint32_t input_len);

/**
 * @brief Converts an unsigned integer to a big-endian byte array.
 *
 * NEW-5: widened from uint32_t to uint64_t so that 64-bit Ethereum fields
 * (nonce, gasLimit, value, maxFeePerGas, maxPriorityFeePerGas) are not
 * silently truncated. Returns up to 8 bytes; @p str must be >= 8 bytes.
 *
 * @param[out] str Pointer to output buffer (at least 8 bytes).
 * @param[in]  val Unsigned 64-bit integer to convert.
 * @return Number of bytes used in the output array (1..8).
 */
uint32_t ConvertNumberToUintArray(uint8_t *str, uint64_t val);

/**
 * @brief Removes leading zeros from a byte array.
 *
 * Bounds-checked: returns 0 if @p out_cap is smaller than the trimmed
 * length. (Trimmed length is always <= @p in_len.)
 *
 * @param[out] out     Pointer to buffer to store trimmed result.
 * @param[in]  out_cap Capacity of @p out in bytes.
 * @param[in]  in      Pointer to input byte array.
 * @param[in]  in_len  Length of input array.
 * @return Number of bytes written into out, or 0 on overflow.
 */
size_t trimLeadingZeros(uint8_t* out, size_t out_cap, const uint8_t* in, size_t in_len);

/**
 * @brief Convert a hexadecimal character to a byte value.
 *
 * Returns -1 on any non-hex character so callers can distinguish a true '0'
 * from a parse error. Previously this returned 0 on invalid input, causing
 * silent data corruption (security audit M-07).
 *
 * @param c Hex character.
 * @return 0..15 for valid hex digits, -1 otherwise.
 */
int fromHex(char c);

/**
 * @brief Convert a hex string to a byte array.
 *
 * Validates that @p hex contains at least @c 2*len characters before any read,
 * to prevent out-of-bounds reads on malformed / truncated RPC responses
 * (security audit H-03).
 *
 * @param hex Input hex string (must be NUL-terminated).
 * @param out Output byte array (caller-allocated, at least @p len bytes).
 * @param len Number of bytes to write to @p out.
 * @return true on success, false if @p hex is too short or NULL.
 */
bool hexToBytes(const char* hex, uint8_t* out, size_t len);

#endif /* UTIL_H */
