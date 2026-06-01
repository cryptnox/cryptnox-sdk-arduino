/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file ArduinoCryptoProvider.cpp
 * @brief Implementation of the Arduino UNO R4 concrete crypto provider.
 *
 * Stitches together the four crypto libraries listed in the header
 * (AESLib / Crypto / micro-ecc / trng) and forwards each
 * @ref CW_CryptoProvider operation to the appropriate backend. Full API
 * documentation lives on the declarations in @ref ArduinoCryptoProvider.h.
 */

#include "ArduinoCryptoProvider.h"
#include <trng.h>
#if CW_VERIFY_CERT
#include <SHA256.h>
#endif
#include <SHA512.h>
#include <AES.h>

/**
 * @brief Construct the provider and register the TRNG-backed RNG with micro-ecc.
 */
ArduinoCryptoProvider::ArduinoCryptoProvider() {
    uECC_set_rng(&ArduinoCryptoProvider::trngCallback);
}

/**
 * @brief Map a portable @ref CW_Curve identifier to the matching micro-ecc descriptor.
 *
 * @c uECC_secp256k1() is only available when micro-ecc is compiled with
 * @c uECC_SUPPORTS_secp256k1=1. The Arduino memory-optimisation step
 * (`scripts\\enable_memory_optimization.bat`) defines that to 0 to drop
 * the curve and shave flash, so we guard the call — secp256k1 keys are
 * still usable on the card itself (the card signs internally); the
 * Arduino side only needs secp256r1 for the secure-channel ECDH.
 */
const uECC_Curve_t* ArduinoCryptoProvider::toUEccCurve(CW_Curve curve) {
    switch (curve) {
        case CW_CURVE_SECP256R1: return uECC_secp256r1();
#if uECC_SUPPORTS_secp256k1
        case CW_CURVE_SECP256K1: return uECC_secp256k1();
#endif
        default:                 return NULL;
    }
}

/**
 * @brief Read one random byte from the RA4M1 hardware TRNG (lazy init on first call).
 */
uint8_t ArduinoCryptoProvider::trngByte() {
    /* Lazy init: the RA4M1 TRNG peripheral is enabled on first read so
     * the constructor stays cheap and order-of-construction-safe with
     * other global objects in the sketch. */
    static bool initialized = false;
    if (!initialized) {
        TRNG.begin();
        initialized = true;
    }
    uint8_t out = 0U;
    TRNG.random8(&out);
    return out;
}

/**
 * @brief Static RNG callback matching the micro-ecc `uECC_RNG_Function` signature.
 */
int ArduinoCryptoProvider::trngCallback(uint8_t* dest, unsigned size) {
    int ret = 0;
    if ((dest != NULL) && (size > 0U)) {
        for (unsigned i = 0U; i < size; i++) {
            dest[i] = trngByte();
        }
        ret = 1;
    }
    return ret;
}

/**
 * @brief Compute SHA-256 (no-op when @c CW_VERIFY_CERT is 0 to drop the dependency).
 */
bool ArduinoCryptoProvider::sha256(const uint8_t* data, size_t len, uint8_t* out) {
    if (out == NULL) { return false; }
#if CW_VERIFY_CERT
    if ((data == NULL) && (len > 0U)) { return false; }
    SHA256 sha;
    sha.update(data, len);
    sha.finalize(out, 32U);
#else
    /* Card-certificate verification compiled out: keep the symbol but skip
     * the work so the SHA256 dependency is not pulled in by the linker. */
    (void)data; (void)len; (void)out;
#endif
    return true;
}

/**
 * @brief Compute SHA-512 over a contiguous buffer.
 */
bool ArduinoCryptoProvider::sha512(const uint8_t* data, size_t len, uint8_t* out) {
    if (out == NULL) { return false; }
    if ((data == NULL) && (len > 0U)) { return false; }
    SHA512 sha;
    sha.update(data, len);
    sha.finalize(out, 64U);
    return true;
}

/**
 * @brief AES-CBC encrypt (selectable bit / null padding).
 */
uint16_t ArduinoCryptoProvider::aesCbcEncrypt(const uint8_t* in, uint16_t len, uint8_t* out,
                                               const uint8_t* key, uint8_t keyLen,
                                               uint8_t* iv, bool bitPadding) {
    _aes.set_paddingmode(bitPadding ? paddingMode::Bit : paddingMode::Null);
    return _aes.encrypt(reinterpret_cast<const byte*>(in), len, out,
                        reinterpret_cast<const byte*>(key), static_cast<int>(keyLen), iv);
}

/**
 * @brief AES-CBC decrypt (selectable bit / null padding).
 */
uint16_t ArduinoCryptoProvider::aesCbcDecrypt(uint8_t* in, uint16_t len, uint8_t* out,
                                               const uint8_t* key, uint8_t keyLen,
                                               uint8_t* iv, bool bitPadding) {
    _aes.set_paddingmode(bitPadding ? paddingMode::Bit : paddingMode::Null);
    return _aes.decrypt(in, len, out,
                        reinterpret_cast<const byte*>(key), static_cast<int>(keyLen), iv);
}

/**
 * @brief ECDH shared-secret computation via micro-ecc.
 */
bool ArduinoCryptoProvider::ecdh(const uint8_t* pubKey, const uint8_t* privKey,
                                  uint8_t* secret, CW_Curve curve) {
    const uECC_Curve_t* uc = toUEccCurve(curve);
    if (uc == NULL) { return false; }
    return (uECC_shared_secret(pubKey, privKey, secret, uc) != 0);
}

/**
 * @brief Generate a fresh EC keypair via micro-ecc (uses the RA4M1 TRNG).
 */
bool ArduinoCryptoProvider::makeKey(uint8_t* pubKey, uint8_t* privKey,
                                     CW_Curve curve) {
    const uECC_Curve_t* uc = toUEccCurve(curve);
    if (uc == NULL) { return false; }
    return (uECC_make_key(pubKey, privKey, uc) != 0);
}

/**
 * @brief Fill a buffer with hardware-TRNG random bytes.
 */
bool ArduinoCryptoProvider::random(uint8_t* dest, unsigned size) {
    return (trngCallback(dest, size) == 1);
}

/**
 * @brief ECDSA verify (raw r||s, 64 bytes) via micro-ecc.
 */
bool ArduinoCryptoProvider::ecdsaVerify(const uint8_t* pubKey64,
                                        const uint8_t* hash, size_t hashLen,
                                        const uint8_t* sig, CW_Curve curve) {
    const uECC_Curve_t* uc = toUEccCurve(curve);
    if (uc == NULL) { return false; }
    return (uECC_verify(pubKey64, hash, static_cast<unsigned>(hashLen), sig, uc) != 0);
}
