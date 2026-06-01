/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file UsdcSigning.ino
 * @example UsdcSigning.ino
 * @brief Send an ERC20 USDC transaction on Ethereum (EIP-1559) via Arduino,
 *        signed on-device using a Cryptnox smart card over NFC (PN532).
 *
 * Demonstrates:
 * - Connecting to WiFi
 * - RLP encoding of an unsigned and signed EIP-1559 transaction
 * - Keccak-256 hashing
 * - Signing the transaction hash with a Cryptnox card via PN532
 * - Determining yParity via the Ethereum ecrecover precompile (eth_call)
 * - Sending the signed transaction via JSON-RPC
 */

#include <Arduino.h>
#include <SPI.h>
#include "keccak256.h"
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include "util.h"
#include "config.h"
#include <CryptnoxWallet.h>

/** @brief PN532 SPI slave-select pin. */
#define PN532_SS_PIN  (10U)

/* Fallback PIN — define CARD_PIN and CARD_PIN_LEN in config.h to override.
 * L-04: hardcoded PIN lives in flash, recoverable via SWD/JTAG — OK for
 * demo, not for prod. See config.template.h for safer patterns. */
#ifndef CARD_PIN
#  define CARD_PIN     "000000000"
#  define CARD_PIN_LEN (9U)
#endif

/* Default RPC path — PublicNode accepts JSON-RPC at root.
 * Infura requires /v3/{PROJECT_ID}: define RPC_PATH in config.h for that case. */
#ifndef RPC_PATH
#  define RPC_PATH "/"
#endif

/* M-04: TLS server-certificate pinning.
 *
 * Without setCACert(), WiFiSSLClient accepts ANY certificate the RPC endpoint
 * presents — a network attacker can MITM the connection and feed crafted
 * nonces / gas prices to make the device sign incorrect transactions, or
 * exfiltrate the basic-auth credentials sent to Infura.
 *
 * The default below is ISRG Root X1 (Let's Encrypt), which signs the chains
 * used by PublicNode (the template's default provider). For a different
 * provider (Infura/DigiCert, Cloudflare, etc.), override WIFI_CA_CERT in
 * config.h with the appropriate root in PEM form. */
#ifndef WIFI_CA_CERT
#  define WIFI_CA_CERT \
"-----BEGIN CERTIFICATE-----\n" \
"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\n" \
"-----END CERTIFICATE-----\n"
#endif

/* L-05 — PRODUCTION: USB-CDC Serial is readable by any host process.
 * Logs leak RPC URL, basic-auth header, nonce, recipient, tx hash, and
 * (if CW_DEBUG_LOGGING=1) secure-channel ciphertext + IVs. Before ship:
 * swap for `NullLoggerAdapter serialAdapter;` and drop Serial.begin(). */
ArduinoLoggerAdapter  serialAdapter;
ArduinoCryptoProvider cryptoProvider;
ArduinoPlatform       platform;
PN532Adapter          nfc(serialAdapter, PN532_SS_PIN, &SPI);
CryptnoxWallet        wallet(nfc, serialAdapter, cryptoProvider, platform);

/** ERC-20 transfer(address,uint256) function selector: keccak256("transfer(address,uint256)")[0..3] */
#define ERC20_TRANSFER_SEL_0  0xa9U
#define ERC20_TRANSFER_SEL_1  0x05U
#define ERC20_TRANSFER_SEL_2  0x9cU
#define ERC20_TRANSFER_SEL_3  0xbbU

#define ERC20_INDEX_OFFSET 64U

/** @brief Sentinel returned by determineYParity() when recovery fails. */
#define YPARITY_UNKNOWN 0xFFU

/** @brief Expected HTTP 200 OK status code. */
#define HTTP_OK 200

/** @brief Maximum number of send-transaction attempts before giving up. */
#define TX_MAX_RETRIES       3U
/** @brief Delay in ms between send-transaction retry attempts. */
#define TX_RETRY_DELAY_MS 2000U
/** @brief Maximum WiFi reconnect poll iterations (each iteration waits 500 ms). */
#define WIFI_RETRY_MAX      20U
/** @brief Buffer size for a two-hex-char + NUL string used in byte-to-hex conversion. */
#define HEX_CHAR_BUF_SIZE    3U
/** @brief Number of leading zero hex characters in the ecrecover v-field padding. */
#define ECRECOVER_V_PAD_CHARS 62U
/** @brief Base value for Ethereum ecrecover v parameter (yParity=0 → v=27, yParity=1 → v=28). */
#define ECRECOVER_V_BASE    27U

/**
 * @brief Ethereum EIP-1559 transaction structure.
 */
struct Tx2 {
    uint64_t nonce;                 /**< Transaction nonce */
    uint64_t maxPriorityFeePerGas;  /**< Max priority fee (wei) */
    uint64_t maxFeePerGas;          /**< Max fee (wei) */
    uint64_t gasLimit;              /**< Gas limit */
    const char* to;                 /**< Recipient address */
    uint64_t value;                 /**< Value in wei */
    const uint8_t* data;            /**< Transaction calldata */
    size_t dataLen;                 /**< Length of calldata */
    uint32_t chainId;               /**< Ethereum chain ID */
};

static const char hexChars[] = "0123456789abcdef";

static void printHex(const char* label, const uint8_t* data, size_t len) {
    Serial.print(label);
    Serial.print(F(": 0x"));
    char buf[3]; buf[2] = '\0';
    for (size_t i = 0; i < len; i++) {
        buf[0] = hexChars[data[i] >> 4];
        buf[1] = hexChars[data[i] & 0x0f];
        Serial.print(buf);
    }
    Serial.println();
}

#if defined(RPC_PROJECT_ID) && defined(RPC_API_SECRET)
/** @brief Maximum byte length of the raw credential string "project_id:secret" + NUL. */
#define AUTH_CRED_BUF_SIZE    128U
/** @brief Buffer size for the full "Basic <base64>" header value + NUL. */
#define AUTH_HEADER_BUF_SIZE  200U

/* NEW-3: base64Encode now requires the output capacity and returns false
 * if it cannot write the full (ceil(inputLen/3)*4 + 1) bytes. Prevents
 * silent stack overflow if AUTH_HEADER_BUF_SIZE is later reduced or a
 * caller passes a too-small buffer. */
static bool base64Encode(const char* input, size_t inputLen, char* output, size_t outputCap) {
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const size_t required = (((inputLen + 2U) / 3U) * 4U) + 1U; /* +1 for NUL */
    if (outputCap < required) {
        return false;
    }
    size_t i = 0U;
    size_t o = 0U;
    while (i < inputLen) {
        uint32_t u0 = static_cast<uint32_t>(static_cast<uint8_t>(input[i]));
        uint32_t u1 = 0U;
        uint32_t u2 = 0U;
        if ((i + 1U) < inputLen) {
            u1 = static_cast<uint32_t>(static_cast<uint8_t>(input[i + 1U]));
        }
        if ((i + 2U) < inputLen) {
            u2 = static_cast<uint32_t>(static_cast<uint8_t>(input[i + 2U]));
        }
        output[o] = kAlphabet[static_cast<uint8_t>(u0 >> 2U)];
        o++;
        output[o] = kAlphabet[static_cast<uint8_t>(((u0 & 0x03U) << 4U) | (u1 >> 4U))];
        o++;
        if ((i + 1U) < inputLen) {
            output[o] = kAlphabet[static_cast<uint8_t>(((u1 & 0x0FU) << 2U) | (u2 >> 6U))];
        } else {
            output[o] = '=';
        }
        o++;
        if ((i + 2U) < inputLen) {
            output[o] = kAlphabet[static_cast<uint8_t>(u2 & 0x3FU)];
        } else {
            output[o] = '=';
        }
        o++;
        i += 3U;
    }
    output[o] = '\0';
    return true;
}

static void buildBasicAuthHeader(char* buf, size_t bufSize) {
    static const char kPrefix[] = "Basic ";
    const size_t prefixLen = sizeof(kPrefix) - 1U;
    char cred[AUTH_CRED_BUF_SIZE];
    // cppcheck-suppress invalidPrintfArgType_s -- macros are char* when defined in config.h; --force evaluates this block without knowing their types
    int written = snprintf(cred, sizeof(cred), "%s:%s", RPC_PROJECT_ID, RPC_API_SECRET);
    /* H-04: snprintf returns the number of bytes it WOULD have written
     * (excluding NUL). A value >= sizeof(cred) means the project id +
     * secret were truncated — sending a truncated Authorization header
     * causes opaque 401s and could leak credentials in retry traces. */
    if ((written < 0) || ((size_t)written >= sizeof(cred))) {
        Serial.println(F("[fatal] RPC_PROJECT_ID + RPC_API_SECRET exceed AUTH_CRED_BUF_SIZE"));
        while (true) { /* halt — do not send a truncated basic-auth header */ }
    }
    /* NEW-1: ensure the output buffer can hold prefix + base64(cred) + NUL.
     * base64 expands by ~4/3; the helper rejects undersized buffers. */
    if (prefixLen >= bufSize) {
        Serial.println(F("[fatal] AUTH_HEADER_BUF_SIZE too small for prefix"));
        while (true) {}
    }
    (void)CW_Utils::safe_memcpy(reinterpret_cast<uint8_t*>(buf), bufSize,
                                reinterpret_cast<const uint8_t*>(kPrefix), prefixLen);
    if (!base64Encode(cred, strlen(cred), buf + prefixLen, bufSize - prefixLen)) {
        Serial.println(F("[fatal] base64 output exceeds AUTH_HEADER_BUF_SIZE"));
        while (true) {}
    }
}
#endif /* defined(RPC_PROJECT_ID) && defined(RPC_API_SECRET) */

static bool ensureWiFi() {
    if (WiFi.status() == WL_CONNECTED) return true;
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint8_t retries = WIFI_RETRY_MAX;
    while ((retries > 0U) && (WiFi.status() != WL_CONNECTED)) {
        retries--;
        delay(500U);
    }
    return WiFi.status() == WL_CONNECTED;
}

/* Each RlpEncodeItem call returns 0 on overflow; bail out immediately so
 * the caller sees rlpLen == 0 and halts before broadcasting garbage. */
#define RLP_ITEM_OR_FAIL(BUF, CAP, OFF, IN, IN_LEN)                          \
    do {                                                                    \
        uint32_t _w = RlpEncodeItem((BUF) + (OFF), (CAP) - (OFF),           \
                                    (IN), (IN_LEN));                        \
        if (_w == 0U) { return 0U; }                                        \
        (OFF) += _w;                                                        \
    } while (0)

static size_t rlpEncodeTxBody(uint8_t* buf, size_t bufCap, const Tx2& tx) {
    size_t off = 0;
    uint8_t tmp[8];
    size_t tmpLen;
    tmpLen = ConvertNumberToUintArray(tmp, tx.chainId);
    RLP_ITEM_OR_FAIL(buf, bufCap, off, tmp, (uint32_t)tmpLen);
    tmpLen = ConvertNumberToUintArray(tmp, tx.nonce);
    RLP_ITEM_OR_FAIL(buf, bufCap, off, tmp, (uint32_t)tmpLen);
    tmpLen = ConvertNumberToUintArray(tmp, tx.maxPriorityFeePerGas);
    RLP_ITEM_OR_FAIL(buf, bufCap, off, tmp, (uint32_t)tmpLen);
    tmpLen = ConvertNumberToUintArray(tmp, tx.maxFeePerGas);
    RLP_ITEM_OR_FAIL(buf, bufCap, off, tmp, (uint32_t)tmpLen);
    tmpLen = ConvertNumberToUintArray(tmp, tx.gasLimit);
    RLP_ITEM_OR_FAIL(buf, bufCap, off, tmp, (uint32_t)tmpLen);
    uint8_t addr[20];
    if (!hexToBytes(tx.to, addr, 20)) {
        Serial.println(F("[fatal] tx.to is not a valid 40-char hex string"));
        while (true) { /* halt to avoid broadcasting a malformed transaction */ }
    }
    RLP_ITEM_OR_FAIL(buf, bufCap, off, addr, 20U);
    tmpLen = ConvertNumberToUintArray(tmp, tx.value);
    RLP_ITEM_OR_FAIL(buf, bufCap, off, tmp, (uint32_t)tmpLen);
    RLP_ITEM_OR_FAIL(buf, bufCap, off, tx.data, (uint32_t)tx.dataLen);
    if (off >= bufCap) { return 0U; }   /* room for the access-list terminator 0xC0 */
    buf[off++] = 0xC0;
    return off;
}

static size_t rlpFinalize(uint8_t* out, size_t outCap, const uint8_t* buf, size_t off) {
    uint8_t header[8];
    size_t header_len = RlpEncodeWholeHeader(header, sizeof(header), off);
    if (header_len == 0U) { return 0U; }
    /* Reject early if the total wouldn't fit (1 type byte + header + body). */
    if ((1U + header_len + off) > outCap) {
        return 0U;
    }
    size_t out_off = 0U;
    out[out_off++] = 0x02;
    (void)CW_Utils::safe_memcpy(out + out_off, outCap - out_off, header, header_len);
    out_off += header_len;
    (void)CW_Utils::safe_memcpy(out + out_off, outCap - out_off, buf, off);
    return out_off + off;
}

size_t rlpEncodeUnsignedTx(const Tx2& tx, uint8_t* out, size_t outCap) {
    uint8_t buf[1024];
    size_t off = rlpEncodeTxBody(buf, sizeof(buf), tx);
    if (off == 0U) { return 0U; }
    return rlpFinalize(out, outCap, buf, off);
}

size_t rlpEncodeSignedTx(const Tx2& tx, const uint8_t* r, const uint8_t* s, const uint8_t* v,
                         uint8_t* out, size_t outCap) {
    uint8_t buf[1024];
    size_t off = rlpEncodeTxBody(buf, sizeof(buf), tx);
    if (off == 0U) { return 0U; }
    uint32_t w;
    w = RlpEncodeItem(buf + off, sizeof(buf) - off, v, 1U);
    if (w == 0U) { return 0U; }
    off += w;
    uint8_t tmp_r[32];
    size_t tmp_len = trimLeadingZeros(tmp_r, sizeof(tmp_r), r, 32U);
    if (tmp_len == 0U) { return 0U; }
    w = RlpEncodeItem(buf + off, sizeof(buf) - off, tmp_r, (uint32_t)tmp_len);
    if (w == 0U) { return 0U; }
    off += w;
    uint8_t tmp_s[32];
    tmp_len = trimLeadingZeros(tmp_s, sizeof(tmp_s), s, 32U);
    if (tmp_len == 0U) { return 0U; }
    w = RlpEncodeItem(buf + off, sizeof(buf) - off, tmp_s, (uint32_t)tmp_len);
    if (w == 0U) { return 0U; }
    off += w;
    size_t ret = rlpFinalize(out, outCap, buf, off);
    /* L-01: wipe the signature scratch buffers for hygiene uniformity with
     * the rest of the codebase. Signatures are public (broadcast on-chain)
     * so this is style/consistency only, not a real secret leak. */
    CW_Utils::secure_wipe(tmp_r, sizeof(tmp_r));
    CW_Utils::secure_wipe(tmp_s, sizeof(tmp_s));
    return ret;
}

/**
 * @brief Encode calldata for ERC-20 transfer(address to, uint256 amount).
 * @param out Output buffer (at least 68 bytes)
 * @return Calldata length (always 68)
 */
size_t encodeERC20Transfer(uint8_t* out) {
    out[0] = ERC20_TRANSFER_SEL_0; /* transfer(address,uint256) selector byte 0 */
    out[1] = ERC20_TRANSFER_SEL_1; /* transfer(address,uint256) selector byte 1 */
    out[2] = ERC20_TRANSFER_SEL_2; /* transfer(address,uint256) selector byte 2 */
    out[3] = ERC20_TRANSFER_SEL_3; /* transfer(address,uint256) selector byte 3 */
    CW_Utils::secure_wipe(out+4,  12U); /* bytes  4-15: ABI word padding before address (12 zero bytes) */
    if (!hexToBytes(ADDR_TO, out+16, 20)) {  /* bytes 16-35: recipient address (20 bytes)           */
        Serial.println(F("[fatal] ADDR_TO is not a valid 40-char hex string"));
        while (true) { /* halt to avoid broadcasting a wrong-recipient transfer */ }
    }
    CW_Utils::secure_wipe(out+36, 28U); /* bytes 36-63: ABI word padding before amount  (28 zero bytes) */
    out[ERC20_INDEX_OFFSET]   = (uint8_t)((AMOUNT_USDC >> 24U) & 0xFFU);
    out[ERC20_INDEX_OFFSET+1] = (uint8_t)((AMOUNT_USDC >> 16U) & 0xFFU);
    out[ERC20_INDEX_OFFSET+2] = (uint8_t)((AMOUNT_USDC >> 8U)  & 0xFFU);
    out[ERC20_INDEX_OFFSET+3] = (uint8_t)( AMOUNT_USDC         & 0xFFU);
    return 68;
}

/**
 * @brief Send a raw signed transaction via JSON-RPC.
 * @param raw Signed transaction bytes
 * @param len Length of raw transaction in bytes
 * @return true if the RPC call succeeded without an error field, false otherwise.
 */
bool sendRawTx(const uint8_t* raw, size_t len) {
    static const char requestPrefix[] =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_sendRawTransaction\","
        "\"params\":[\"0x";
    /* Fixed closing of the JSON-RPC body that follows the hex transaction bytes. */
    static const char requestSuffix[] = "\"]}";
    bool sent = false;
    for (uint8_t attempt = 0U; (attempt < TX_MAX_RETRIES) && !sent; attempt++) {
        if (attempt != 0U) {
            delay(TX_RETRY_DELAY_MS);
        }
        if (!ensureWiFi()) {
            Serial.println(F("sendRawTx: WiFi reconnect failed"));
            continue;
        }
        WiFiSSLClient wifiClient;
#ifndef WIFI_DISABLE_CA_PINNING
        /* M-04: pin the RPC server's CA so a network MITM cannot serve a
         * forged certificate and feed crafted nonces/gas prices.
         * Define WIFI_DISABLE_CA_PINNING in config.h to skip this — DEV ONLY,
         * leaves the connection vulnerable to MITM. */
        wifiClient.setCACert(WIFI_CA_CERT);
#endif
        HttpClient client(wifiClient, RPC_HOST, RPC_PORT);
        client.beginRequest();
        int err = client.post(RPC_PATH);
        if (err != HTTP_SUCCESS) {
            Serial.print(F("sendRawTx: POST failed, err="));
            Serial.println(err);
            client.stop();
            continue;
        }
        /* Content-Length = prefix + 2 hex chars per raw byte + suffix. */
        client.sendHeader("Content-Type", "application/json");
        client.sendHeader("Content-Length",
            (int)(sizeof(requestPrefix)-1) + 2*(int)len + (int)(sizeof(requestSuffix)-1));
#if defined(RPC_PROJECT_ID) && defined(RPC_API_SECRET)
        {
            char authBuf[AUTH_HEADER_BUF_SIZE];
            buildBasicAuthHeader(authBuf, sizeof(authBuf));
            client.sendHeader("Authorization", authBuf);
        }
#endif
        client.beginBody();
        client.print(requestPrefix);
        /* Encode each raw transaction byte as two hex characters and stream it
         * directly to the HTTP client, avoiding a large intermediate buffer. */
        char byteHexStr[HEX_CHAR_BUF_SIZE];
        byteHexStr[2] = '\0';
        for (size_t i = 0; i < len; i++) {
            byteHexStr[0] = hexChars[raw[i] >> 4];   /* high nibble */
            byteHexStr[1] = hexChars[raw[i] & 0x0f]; /* low nibble  */
            client.print(byteHexStr);
        }
        client.print(requestSuffix);
        client.endRequest();
        int status = client.responseStatusCode();
        String responseBody = client.responseBody();
        Serial.print(F("[RPC] HTTP ")); Serial.println(status);
        /* NEW-2: dumping the full RPC response leaks tx metadata (nonce,
         * recipient, gas) over USB-CDC. Gate behind CW_DEBUG_LOGGING so a
         * production build (CW_DEBUG_LOGGING=0) stays silent. */
#if CW_DEBUG_LOGGING
        Serial.print(F("[RPC] ")); Serial.println(responseBody);
#endif
        bool statusOk = (status == HTTP_OK);
        /* L-03 (accepted): coarse substring search instead of a JSON parser.
         * Not a security issue — at worst a false positive triggers a retry.
         * ArduinoJson would cost +15-25 KB flash for marginal robustness. */
        bool noJsonError = (responseBody.indexOf("\"error\"") == -1);
        sent = statusOk && noJsonError;
        /* eth_sendRawTransaction returns {"jsonrpc":"2.0","id":1,"result":"0x<txhash>"}.
         * The tx hash is on-chain public info — extract and print so the user can
         * track the broadcast on a block explorer. */
        if (sent) {
            int r = responseBody.indexOf("\"result\":\"0x");
            if (r >= 0) {
                int start = r + 10; /* skip past "\"result\":\"" */
                int end = responseBody.indexOf("\"", start);
                if (end > start) {
                    Serial.print(F("[tx] hash="));
                    Serial.println(responseBody.substring(start, end));
                }
            }
        }
        client.stop();
    }
    return sent;
}

/**
 * @brief Determine EIP-1559 yParity by calling the Ethereum ecrecover precompile.
 *
 * Tries v=27 (yParity=0) and v=28 (yParity=1). Compares the recovered address
 * with ADDR_FROM (defined in config.h) to pick the correct value.
 *
 * @param hash    Keccak-256 transaction hash (32 bytes).
 * @param r       Signature r component (32 bytes).
 * @param s       Signature s component (32 bytes).
 * @return 0 or 1 on success, YPARITY_UNKNOWN on failure.
 */
uint8_t determineYParity(const uint8_t* hash, const uint8_t* r, const uint8_t* s) {
    /* ecrecover calldata: "0x" + hash(64) + v(64) + r(64) + s(64) = 258 chars + NUL */
    char hexBuf[260];
    uint16_t pos = 0U;
    hexBuf[pos++] = '0';
    hexBuf[pos++] = 'x';
    for (uint8_t i = 0U; i < 32U; i++) {
        hexBuf[pos++] = hexChars[hash[i] >> 4];
        hexBuf[pos++] = hexChars[hash[i] & 0x0f];
    }
    /* v field: ECRECOVER_V_PAD_CHARS zero chars, then 1 value byte — filled per iteration */
    const uint16_t vOffset = pos;
    for (uint8_t i = 0U; i < ECRECOVER_V_PAD_CHARS; i++) {
        hexBuf[pos++] = '0';
    }
    pos += 2U; /* placeholder for v byte */
    for (uint8_t i = 0U; i < 32U; i++) {
        hexBuf[pos++] = hexChars[r[i] >> 4];
        hexBuf[pos++] = hexChars[r[i] & 0x0f];
    }
    for (uint8_t i = 0U; i < 32U; i++) {
        hexBuf[pos++] = hexChars[s[i] >> 4];
        hexBuf[pos++] = hexChars[s[i] & 0x0f];
    }
    hexBuf[pos] = '\0'; /* pos == 258 */

    static const char requestPrefix[] =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_call\","
        "\"params\":[{\"to\":\"0x0000000000000000000000000000000000000001\","
        "\"data\":\"";
    static const char requestSuffix[] = "\"},\"latest\"]}";
    const int bodyLen = (int)(sizeof(requestPrefix) - 1) + 258 + (int)(sizeof(requestSuffix) - 1);

    uint8_t result = YPARITY_UNKNOWN;
    for (uint8_t yp = 0U; (yp <= 1U) && (result == YPARITY_UNKNOWN); yp++) {
        /* Patch v byte into last two chars of the v field.
         * Ethereum ecrecover: v=27 means yParity=0, v=28 means yParity=1. */
        const uint8_t v = ECRECOVER_V_BASE + yp;
        hexBuf[vOffset + ECRECOVER_V_PAD_CHARS]      = hexChars[(v & 0xFFU) >> 4U];
        hexBuf[vOffset + ECRECOVER_V_PAD_CHARS + 1U] = hexChars[(v & 0xFFU) & 0x0FU];

        if (!ensureWiFi()) {
            Serial.println(F("determineYParity: WiFi reconnect failed"));
            continue;
        }
        WiFiSSLClient wifiClient;
#ifndef WIFI_DISABLE_CA_PINNING
        /* M-04: pin the RPC server's CA so a network MITM cannot serve a
         * forged certificate and feed crafted nonces/gas prices.
         * Define WIFI_DISABLE_CA_PINNING in config.h to skip this — DEV ONLY,
         * leaves the connection vulnerable to MITM. */
        wifiClient.setCACert(WIFI_CA_CERT);
#endif
        HttpClient client(wifiClient, RPC_HOST, RPC_PORT);
        client.beginRequest();
        int err = client.post(RPC_PATH);
        if (err != HTTP_SUCCESS) {
            Serial.print(F("determineYParity: POST failed, err="));
            Serial.println(err);
            client.stop();
            continue;
        }
        client.sendHeader("Content-Type", "application/json");
        client.sendHeader("Content-Length", bodyLen);
#if defined(RPC_PROJECT_ID) && defined(RPC_API_SECRET)
        {
            char authBuf[AUTH_HEADER_BUF_SIZE];
            buildBasicAuthHeader(authBuf, sizeof(authBuf));
            client.sendHeader("Authorization", authBuf);
        }
#endif
        client.beginBody();
        client.print(requestPrefix);
        client.print(hexBuf);
        client.print(requestSuffix);
        client.endRequest();

        int status = client.responseStatusCode();
        String response = client.responseBody(); /* consume response body */
        client.stop();
        if (status != HTTP_OK) {
            continue;
        }
        int resultIdx = response.indexOf("\"result\"");
        if (resultIdx < 0) {
            continue;
        }
        int hexIdx = response.indexOf("0x", resultIdx);
        if (hexIdx < 0) {
            continue;
        }
        /* H-05: validate the response is long enough before substring(). ecrecover
         * returns a 32-byte (64-hex-char) word; with the "0x" prefix the slice
         * spans hexIdx+2 .. hexIdx+66. A truncated RPC response would silently
         * give us an empty/garbage recovered address and let the loop progress. */
        if (response.length() < (unsigned int)(hexIdx + 66)) {
            continue;
        }
        /* ecrecover returns 32-byte word; address = last 20 bytes = last 40 hex chars */
        String recovered = response.substring(hexIdx + 26, hexIdx + 66);
        Serial.print(F("[ecrecover] v=")); Serial.print(v);
        Serial.print(F(" recovered=0x")); Serial.println(recovered);
        Serial.print(F("[ecrecover] expected=0x")); Serial.println(F(ADDR_FROM));
        if (recovered.equalsIgnoreCase(ADDR_FROM)) {
            result = yp;
        }
    }
    return result;
}

/**
 * @brief Fetch the current nonce for ADDR_FROM via eth_getTransactionCount.
 * @param nonce Output: nonce value on success (note: 0 is a valid nonce for a fresh address).
 * @return 0 on success, 1 on failure.
 */
uint8_t fetchNonce(uint64_t* nonce) {
    static const char requestPrefix[] =
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"eth_getTransactionCount\","
        "\"params\":[\"0x";
    static const char requestSuffix[] = "\",\"pending\"]}";
    const int bodyLen = (int)(sizeof(requestPrefix)-1) + 40 + (int)(sizeof(requestSuffix)-1);

    uint8_t result = 1U;
    for (uint8_t attempt = 0U; (attempt < TX_MAX_RETRIES) && (result != 0U); attempt++) {
        if (attempt != 0U) {
            delay(1000U);
        }
        if (!ensureWiFi()) {
            Serial.println(F("fetchNonce: WiFi reconnect failed"));
            continue;
        }
        WiFiSSLClient wifiClient;
#ifndef WIFI_DISABLE_CA_PINNING
        /* M-04: pin the RPC server's CA so a network MITM cannot serve a
         * forged certificate and feed crafted nonces/gas prices.
         * Define WIFI_DISABLE_CA_PINNING in config.h to skip this — DEV ONLY,
         * leaves the connection vulnerable to MITM. */
        wifiClient.setCACert(WIFI_CA_CERT);
#endif
        HttpClient client(wifiClient, RPC_HOST, RPC_PORT);
        client.beginRequest();
        Serial.print(F("fetchNonce: connecting to "));
        Serial.print(F(RPC_HOST));
        Serial.println(F(RPC_PATH));
        int err = client.post(RPC_PATH);
        if (err != HTTP_SUCCESS) {
            Serial.print(F("fetchNonce: POST failed, err="));
            Serial.println(err);
            client.stop();
            continue;
        }
        client.sendHeader("Content-Type", "application/json");
        client.sendHeader("Content-Length", bodyLen);
#if defined(RPC_PROJECT_ID) && defined(RPC_API_SECRET)
        {
            char authBuf[AUTH_HEADER_BUF_SIZE];
            buildBasicAuthHeader(authBuf, sizeof(authBuf));
            client.sendHeader("Authorization", authBuf);
        }
#endif
        client.beginBody();
        client.print(requestPrefix);
        client.print(ADDR_FROM);
        client.print(requestSuffix);
        client.endRequest();

        int status = client.responseStatusCode();
        String resp = client.responseBody();
        client.stop();
        if (status == HTTP_OK) {
            int ri = resp.indexOf("\"result\"");
            int xi = (ri >= 0) ? resp.indexOf("0x", ri) : -1;
            if (xi >= 0) {
                uint64_t parsed = 0U;
                /* H-06: cap at 16 hex digits (uint64 capacity). A malicious or
                 * malformed RPC returning a longer hex string would silently
                 * overflow the uint64 and wrap to a small (replayed) nonce. */
                int digitCount = 0;
                bool overflowed = false;
                for (int i = xi + 2; i < (int)resp.length(); i++) {
                    char c = resp[i];
                    if (!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) break;
                    if (digitCount >= 16) { overflowed = true; break; }
                    parsed = (parsed << 4) | fromHex(c);
                    digitCount++;
                }
                /* NEW-4: require >= 1 hex digit so "0x" / "0xZZZ" is not
                 * treated as a valid nonce=0 result on parse failure. */
                if (!overflowed && (digitCount > 0)) {
                    *nonce = parsed;
                    result = 0U;
                }
            }
        }
    }
    return result;
}

/**
 * @brief Arduino setup: init PN532, connect WiFi, build tx, sign with Cryptnox card, send.
 */
void setup() {
    Serial.begin(115200);
    delay(2000);

    /* Init SPI and PN532 */
    SPI.begin();
    if (!wallet.begin()) {
        Serial.println(F("PN532 init failed! Halting."));
        while(1);
    }
    Serial.println(F("PN532 OK"));

#ifdef WIFI_DISABLE_CA_PINNING
    Serial.println(F("⚠️  WIFI_DISABLE_CA_PINNING is set — TLS certificate is NOT validated."));
    Serial.println(F("    Connection is vulnerable to MITM. DEV ONLY, do not use in production."));
#endif

    /* Connect to WiFi */
    Serial.print(F("Connecting to WiFi"));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint8_t retries = 20U;
    while ((retries > 0U) && (WiFi.status() != WL_CONNECTED)) {
        retries--;
        delay(500U);
        Serial.print(F("."));
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi failed!"));
        while(1);
    }
    delay(2000); /* Allow network stack to stabilise before first SSL connection */

    /* Build ERC-20 calldata */
    uint8_t calldata[68];
    size_t calLen = encodeERC20Transfer(calldata);

    /* Build unsigned EIP-1559 transaction */
    Tx2 tx2;
    uint64_t fetchedNonce = 0U;
    if (fetchNonce(&fetchedNonce) != 0U) {
        Serial.println(F("fetchNonce failed! Halting."));
        while(1);
    }
    tx2.nonce              = fetchedNonce;
    tx2.maxPriorityFeePerGas = MAX_PRIORITY_FEE;
    tx2.maxFeePerGas       = MAX_FEE;
    tx2.gasLimit           = GAS_LIMIT_ERC20;
    tx2.to                 = ADDR_USDC;
    tx2.value              = 0;
    tx2.data               = calldata;
    tx2.dataLen            = calLen;
    tx2.chainId            = CHAIN_ID_SEPOLIA;

    /* RLP encode unsigned tx */
    static const size_t kRlpBufSize = 512U;
    uint8_t rlpUnsigned[kRlpBufSize];
    size_t rlpLen = rlpEncodeUnsignedTx(tx2, rlpUnsigned, kRlpBufSize);
    /* L-02: defense-in-depth. A USDC transfer EIP-1559 tx is ~150-200 bytes,
     * so 512 is plenty for the current shape — but if a future change adds
     * an access list, auth list (EIP-7702), or larger tx.data, a silent
     * stack overflow would corrupt the return address. Halt explicitly. */
    /* rlpFinalize returns 0 on overflow (via safe_memcpy bounds check). The
     * post-check stays as defense-in-depth in case a future change skips
     * rlpFinalize's gate. */
    if ((rlpLen == 0U) || (rlpLen > kRlpBufSize)) {
        Serial.print(F("[fatal] rlpUnsigned overflow or empty: ")); Serial.println(rlpLen);
        while (true) {}
    }

    /* Keccak-256 hash */
    uint8_t hashKeccak[32];
    keccak256(static_cast<const uint8_t*>(rlpUnsigned), rlpLen, hashKeccak);
    printHex("[HASH]", hashKeccak, 32);

    /* BIP44 Ethereum path: m/44'/60'/0'/0/0 */
    static const uint8_t eth_path[20] = {
        0x80U,0x00U,0x00U,0x2CU,  /* 44' */
        0x80U,0x00U,0x00U,0x3CU,  /* 60' */
        0x80U,0x00U,0x00U,0x00U,  /* 0'  */
        0x00U,0x00U,0x00U,0x00U,  /* 0   */
        0x00U,0x00U,0x00U,0x00U,  /* 0   */
    };

    /* === Sign with Cryptnox card over NFC (with retry on NFC dropout) === */
    Serial.println(F("Place Cryptnox card on PN532 reader..."));
    CW_SignResult signResult;
    for (uint8_t attempt = 0U; attempt < 3U; attempt++) {
        if (attempt != 0U) {
            delay(1000U);
        }
        CW_SecureSession session;
        if (!wallet.connect(session)) {
            continue;
        }
        Serial.println(F("Card connected, secure channel established."));

        CW_SignRequest signReq(session, CW_SIGN_DERIVE_K1, CW_SIGN_SIG_ECDSA_LOW_S, CW_SIGN_WITH_PIN);
        signReq.hash             = hashKeccak;
        signReq.hashLength       = static_cast<uint8_t>(CW_HASH_SIZE);
        signReq.derivePath       = eth_path;
        signReq.derivePathLength = static_cast<uint8_t>(sizeof(eth_path));
        (void)CW_Utils::safe_memcpy(signReq.pin, sizeof(signReq.pin),
                                    reinterpret_cast<const uint8_t*>(CARD_PIN), CARD_PIN_LEN);

        signResult = wallet.sign(signReq);
        wallet.disconnect(session);
        if (signResult.errorCode == CW_OK) {
            break;
        }
        if (signResult.errorCode == CW_SIGN_PIN_INCORRECT ||
            signResult.errorCode == CW_SIGN_NO_KEY_LOADED) {
            Serial.print(F("Card rejected sign command (error=0x"));
            Serial.print(signResult.errorCode, HEX);
            Serial.println(F(") — check PIN and card initialisation. Halting."));
            /* M-05: explicitly wipe the PIN before halting. The CW_SignRequest
             * destructor would normally do this on scope exit, but the while(1)
             * below stays inside the for() body so the destructor never runs. */
            CW_Utils::secure_wipe(signReq.pin, sizeof(signReq.pin));
            while(1);
        }
        Serial.print(F("Sign attempt "));
        Serial.print(attempt + 1U);
        Serial.print(F(" failed, error=0x"));
        Serial.println(signResult.errorCode, HEX);
    }

    if (signResult.errorCode != CW_OK) {
        Serial.print(F("Sign failed: error=0x"));
        Serial.println(signResult.errorCode, HEX);
        while(1);
    }
    Serial.println(F("Signed."));

    const uint8_t* r = signResult.signature;       /* first 32 bytes */
    const uint8_t* s = signResult.signature + 32;  /* last  32 bytes */
    printHex("[SIG r]", r, 32);
    printHex("[SIG s]", s, 32);

    /* Determine yParity */
    uint8_t yParity = determineYParity(hashKeccak, r, s);
    if (yParity == YPARITY_UNKNOWN) {
        Serial.println(F("yParity determination failed! Halting."));
        while(1);
    }
    Serial.print(F("yParity: "));
    Serial.println(yParity);

    /* RLP encode signed tx and send */
    uint8_t rlpSigned[kRlpBufSize];
    size_t rlpSignedLen = rlpEncodeSignedTx(tx2, r, s, &yParity, rlpSigned, kRlpBufSize);
    /* L-02 + safe_memcpy gate inside rlpFinalize. */
    if ((rlpSignedLen == 0U) || (rlpSignedLen > kRlpBufSize)) {
        Serial.print(F("[fatal] rlpSigned overflow or empty: ")); Serial.println(rlpSignedLen);
        while (true) {}
    }

    Serial.println(F("Sending..."));
    if (sendRawTx(rlpSigned, rlpSignedLen)) {
        Serial.println(F("Transaction sent successfully!"));
    } else {
        Serial.println(F("Transaction FAILED."));
    }
}

/**
 * @brief Arduino loop (empty — transaction is sent once in setup).
 */
void loop() {}
