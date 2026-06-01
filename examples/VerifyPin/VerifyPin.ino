/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file VerifyPin.ino
 * @example VerifyPin.ino
 * @brief Minimal Cryptnox example: open a secure channel and submit a PIN.
 *
 * Wiring & prerequisites:
 *   - PN532 NFC reader on SPI, with SS on pin @ref PN532_SS_PIN.
 *   - A Cryptnox card initialised with a known PIN (use the Cryptnox CLI:
 *     `cryptnox init`, then `cryptnox seed generate`).
 *   - @ref DEMO_PIN must match the PIN set on the card.
 *
 * What the sketch does in each loop iteration:
 *   1. Connect to the card and establish the secure channel.
 *   2. Submit @ref DEMO_PIN over the secure channel.
 *   3. Print "PIN accepted" if the card returned SW 0x9000, otherwise
 *      "PIN rejected (or channel error)". When the SW is not 0x9000 the SDK
 *      also prints the raw bytes (e.g. "Secured APDU: bad SW 0x63C2" means
 *      wrong PIN with 2 retries remaining).
 *   4. Disconnect.
 *
 * @warning Every wrong PIN attempt decrements the card's retry counter.
 *          Reaching 0 retries permanently blocks the PIN; recovery then
 *          requires the PUK via @c cryptnox unblock_pin. Halt the sketch as
 *          soon as @ref CryptnoxWallet::verifyPin returns @c false and check
 *          @ref DEMO_PIN before retrying.
 *
 * @note The PIN "000000000" used by the project examples is a demo
 *       placeholder. In real deployments set a strong PIN, never commit
 *       source files containing a real PIN, and keep this value out of any
 *       version-controlled config.
 */
#include <CryptnoxWallet.h>
#include <SPI.h>

/** @brief SPI slave-select (CS) pin connected to the PN532 module. */
#define PN532_SS_PIN  (10U)

/**
 * @brief Demo PIN used by this example. Must match the PIN that the card
 *        was initialised with (4–9 ASCII digits).
 */
#define DEMO_PIN      "000000000"

/** @brief Arduino logger adapter — emits SDK diagnostics on @c Serial. */
ArduinoLoggerAdapter   serialAdapter;

/** @brief PN532 transport adapter over SPI. */
PN532Adapter           nfc(serialAdapter, PN532_SS_PIN, &SPI);

/** @brief Crypto provider (AES / SHA / micro-ecc / TRNG bridge for Arduino). */
ArduinoCryptoProvider  cryptoProvider;

/** @brief Platform adapter (Arduino blocking delay). */
ArduinoPlatform        platform;

/** @brief High-level Cryptnox wallet wiring the four adapters together. */
CryptnoxWallet         wallet(nfc, serialAdapter, cryptoProvider, platform);

/**
 * @brief Arduino setup hook.
 *
 * Brings up @c Serial, the SPI bus and the PN532 reader. Halts on init
 * failure (no reader detected) so the user can inspect the Serial output.
 */
void setup() {
    serialAdapter.begin(115200);
    delay(1000);                              /* Arduino R4: wait for Serial */
    SPI.begin();

    if (!wallet.begin()) {
        serialAdapter.println(F("PN532 init failed"));
        while (1);
    }
}

/**
 * @brief Arduino main loop.
 *
 * One full secure-channel session per iteration on the happy path:
 *   - @ref CryptnoxWallet::connect opens the channel,
 *   - @ref CryptnoxWallet::verifyPin sends @ref DEMO_PIN,
 *   - @ref CryptnoxWallet::disconnect tears the channel down.
 *
 * If @ref CryptnoxWallet::verifyPin returns @c false the sketch enters an
 * infinite halt. This is deliberate: looping would resubmit the same wrong
 * PIN on every iteration and exhaust the card's retry counter within a few
 * seconds, permanently blocking the PIN.
 *
 * The 1 s delay between successful iterations gives time to remove/replace
 * the card and keeps the Serial output readable.
 */
void loop() {
    CW_SecureSession session;
    if (!wallet.connect(session)) {
        serialAdapter.println(F("Card not detected"));
        wallet.disconnect(session);
        delay(1000);
        return;
    }

    if (!wallet.verifyPin(session,
                          reinterpret_cast<const uint8_t*>(DEMO_PIN),
                          (uint8_t)strlen(DEMO_PIN))) {
        /* CRITICAL: do NOT loop on failure — each wrong PIN attempt
         * decrements the card's retry counter (typically 3-5 tries) and
         * permanently blocks the PIN at zero. Halt and let the developer
         * inspect the Serial output / fix DEMO_PIN before retrying. */
        serialAdapter.println(F("PIN rejected — halting to protect retry counter"));
        wallet.disconnect(session);
        while (1);
    }

    serialAdapter.println(F("PIN accepted"));
    wallet.disconnect(session);
    delay(1000);
}
