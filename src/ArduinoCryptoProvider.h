/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file ArduinoCryptoProvider.h
 * @brief Concrete `CW_CryptoProvider` for the Arduino UNO R4 (RA4M1).
 *
 * Declares @ref ArduinoCryptoProvider, the Arduino-side concrete crypto
 * adapter the host integration injects into @ref CryptnoxWallet. The adapter
 * stitches together four independent libraries — AESLib, the rweather
 * Crypto SHA-256/512 implementations, micro-ecc, and the on-chip RA4M1
 * TRNG — behind the single platform-independent @ref CW_CryptoProvider
 * interface.
 *
 * @see CW_CryptoProvider
 * @see CryptnoxWallet
 */

#ifndef ARDUINOCRYPTOPROVIDER_H
#define ARDUINOCRYPTOPROVIDER_H

#include <Arduino.h>
#include <uECC.h>
#include "cryptnox-sdk-cpp/CW_CryptoProvider.h"
#include "AESLib.h"

/**
 * @class ArduinoCryptoProvider
 * @ingroup arduino_adapters
 * @brief `CW_CryptoProvider` implementation for the Arduino UNO R4 (RA4M1).
 *
 * Provides the primitives the SDK needs to talk to a Cryptnox card:
 *
 * | Operation                  | Backing library / hardware |
 * |----------------------------|----------------------------|
 * | AES-CBC encrypt / decrypt  | [AESLib](https://github.com/suculent/thinx-aes-lib) |
 * | SHA-256 (cert verification, optional via @c CW_VERIFY_CERT) | [Crypto / SHA256](https://rweather.github.io/arduinolibs/crypto.html) |
 * | SHA-512                    | [Crypto / SHA512](https://rweather.github.io/arduinolibs/crypto.html) |
 * | ECDH + EC key generation + ECDSA verify | [micro-ecc](https://github.com/kmackay/micro-ecc) |
 * | Random bytes               | RA4M1 on-chip True-RNG via the `trng` Arduino library |
 *
 * The constructor registers the internal static RNG callback with micro-ecc
 * via @c uECC_set_rng() so the caller never needs to wire up randomness
 * manually — without this, micro-ecc would fall back to its insecure
 * default RNG and ECDH keypairs would be predictable.
 *
 * @par Example
 * @code
 * ArduinoCryptoProvider crypto;       // installs RA4M1 TRNG into micro-ecc
 * CW_CryptoProvider& provider = crypto;
 * uint8_t pub[64], priv[32];
 * provider.makeKey(pub, priv, CW_CURVE_SECP256R1);
 * @endcode
 *
 * @note Non-copyable (RNG callback registration is global; cloning the
 *       provider has no useful semantics).
 * @note When @c CW_VERIFY_CERT is compiled to 0 the SHA-256 entry point is
 *       a no-op so the @c SHA256 dependency can be dropped — the SDK only
 *       needs SHA-256 for card-certificate validation.
 */
class ArduinoCryptoProvider : public CW_CryptoProvider {
public:
    /**
     * @brief Construct the provider and install the RA4M1 TRNG into micro-ecc.
     */
    ArduinoCryptoProvider();

    ArduinoCryptoProvider(const ArduinoCryptoProvider&) = delete;
    ArduinoCryptoProvider& operator=(const ArduinoCryptoProvider&) = delete;

    /** @name CW_CryptoProvider interface */
    ///@{

    /**
     * @brief Compute SHA-256 over a contiguous buffer.
     *
     * Only does the work when @c CW_VERIFY_CERT is non-zero; otherwise the
     * call short-circuits without touching @p out so the `SHA256` symbol
     * is not pulled in by the linker.
     *
     * @param[in]  data Pointer to the data to hash.
     * @param[in]  len  Length of @p data in bytes.
     * @param[out] out  32-byte output buffer.
     * @return @c true on success (including the compiled-out fast path),
     *         @c false on a NULL argument.
     */
    bool sha256(const uint8_t* data, size_t len, uint8_t* out) override;

    /**
     * @brief Compute SHA-512 over a contiguous buffer.
     *
     * Used by the secure channel layer for ECDH-derived session-key
     * derivation; always compiled in.
     *
     * @param[in]  data Pointer to the data to hash.
     * @param[in]  len  Length of @p data in bytes.
     * @param[out] out  64-byte output buffer.
     * @return @c true on success, @c false on a NULL argument.
     */
    bool sha512(const uint8_t* data, size_t len, uint8_t* out) override;

    /**
     * @brief AES-CBC encrypt (selectable bit / null padding).
     *
     * @param[in]     in         Plaintext input buffer.
     * @param[in]     len        Input length in bytes.
     * @param[out]    out        Ciphertext output buffer (caller-allocated).
     * @param[in]     key        Key bytes.
     * @param[in]     keyLen     Key length in bytes (16, 24, or 32).
     * @param[in,out] iv         IV in / next-block IV out (16 bytes).
     * @param[in]     bitPadding @c true → ISO/IEC 7816-4 bit padding, @c
     *                           false → null padding.
     * @return Number of ciphertext bytes written into @p out.
     */
    uint16_t aesCbcEncrypt(const uint8_t* in, uint16_t len, uint8_t* out,
                           const uint8_t* key, uint8_t keyLen,
                           uint8_t* iv, bool bitPadding) override;

    /**
     * @brief AES-CBC decrypt (selectable bit / null padding).
     *
     * @param[in,out] in         Ciphertext input buffer (may be mutated by AESLib).
     * @param[in]     len        Input length in bytes.
     * @param[out]    out        Plaintext output buffer.
     * @param[in]     key        Key bytes.
     * @param[in]     keyLen     Key length in bytes.
     * @param[in,out] iv         IV in / next-block IV out.
     * @param[in]     bitPadding @c true → strip ISO/IEC 7816-4 padding.
     * @return Number of plaintext bytes written into @p out.
     */
    uint16_t aesCbcDecrypt(uint8_t* in, uint16_t len, uint8_t* out,
                           const uint8_t* key, uint8_t keyLen,
                           uint8_t* iv, bool bitPadding) override;

    /**
     * @brief Compute the ECDH shared secret on a portable curve identifier.
     *
     * Translates @p curve to the matching micro-ecc curve descriptor
     * (@c uECC_secp256r1() / @c uECC_secp256k1()) and calls
     * @c uECC_shared_secret().
     *
     * @param[in]  pubKey  Peer public key (X || Y, no leading 0x04).
     * @param[in]  privKey Local private scalar.
     * @param[out] secret  Output buffer for the shared X coordinate.
     * @param[in]  curve   Portable curve identifier (@ref CW_CURVE_SECP256R1
     *                     or @ref CW_CURVE_SECP256K1).
     * @return @c true on success, @c false if the peer's public key is not
     *         on the curve or @p curve is unrecognised.
     */
    bool ecdh(const uint8_t* pubKey, const uint8_t* privKey,
              uint8_t* secret, CW_Curve curve) override;

    /**
     * @brief Generate a fresh EC keypair via micro-ecc.
     *
     * Uses the RA4M1 TRNG installed by the constructor — without that
     * registration, @c uECC_make_key() would silently call its built-in
     * fallback RNG and the resulting key would not be cryptographically
     * secure.
     *
     * @param[out] pubKey  Public key output (X || Y).
     * @param[out] privKey Private scalar output.
     * @param[in]  curve   Portable curve identifier.
     * @return @c true on success, @c false if @p curve is unrecognised or
     *         micro-ecc rejected the generated scalar (negligible
     *         probability — retry is safe).
     */
    bool makeKey(uint8_t* pubKey, uint8_t* privKey,
                 CW_Curve curve) override;

    /**
     * @brief Fill a buffer with random bytes from the RA4M1 hardware TRNG.
     *
     * Single source of randomness for the whole SDK.
     *
     * @param[out] dest Output buffer.
     * @param[in]  size Number of bytes to write.
     * @return @c true if all @p size bytes were generated, @c false on a
     *         NULL pointer or zero size.
     */
    bool random(uint8_t* dest, unsigned size) override;

    /**
     * @brief Verify a raw r||s ECDSA signature against a message hash.
     *
     * Translates @p curve to a micro-ecc descriptor and calls
     * @c uECC_verify().
     *
     * @param[in] pubKey64 Public key (X || Y, 64 bytes).
     * @param[in] hash     Message hash buffer.
     * @param[in] hashLen  Length of the hash in bytes (typically 32).
     * @param[in] sig      Raw r||s signature (64 bytes).
     * @param[in] curve    Portable curve identifier.
     * @return @c true if the signature verifies, @c false otherwise (bad
     *         signature, malformed key, or unrecognised curve).
     */
    bool ecdsaVerify(const uint8_t* pubKey64,
                     const uint8_t* hash, size_t hashLen,
                     const uint8_t* sig, CW_Curve curve) override;
    ///@}

private:
    AESLib _aes; ///< AESLib engine instance reused across all `aesCbc*` calls.

    /**
     * @brief Translate a portable @ref CW_Curve to the matching micro-ecc descriptor.
     *
     * @param[in] curve Portable curve identifier.
     * @return micro-ecc curve descriptor, or @c NULL if @p curve is unknown.
     */
    static const uECC_Curve_t* toUEccCurve(CW_Curve curve);

    /**
     * @brief Generate one random byte from the RA4M1 hardware TRNG.
     *
     * Lazily initialises the @c TRNG global on first call.
     *
     * @return A uniformly random byte in [0, 255].
     */
    static uint8_t trngByte();

    /**
     * @brief Static RNG callback registered with @c uECC_set_rng().
     *
     * @param[out] dest Output buffer.
     * @param[in]  size Number of bytes to produce.
     * @return @c 1 on success, @c 0 on a NULL @p dest or zero @p size.
     */
    static int trngCallback(uint8_t* dest, unsigned size);
};

#endif // ARDUINOCRYPTOPROVIDER_H
