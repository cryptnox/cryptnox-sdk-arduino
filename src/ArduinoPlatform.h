/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file ArduinoPlatform.h
 * @brief Concrete `CW_Platform` over Arduino's `delay()`.
 *
 * Declares @ref ArduinoPlatform, the Arduino-side concrete platform adapter
 * the host integration injects into @ref CryptnoxWallet. The SDK uses the
 * single @ref CW_Platform::sleep_ms primitive to space APDU exchanges
 * across the secure-channel handshake; on Arduino that maps directly to
 * the framework's blocking `delay()` call.
 *
 * @see CW_Platform
 * @see CryptnoxWallet
 */

#ifndef ARDUINOPLATFORM_H
#define ARDUINOPLATFORM_H

#include <Arduino.h>
#include "cryptnox-sdk-cpp/CW_Platform.h"

/**
 * @class ArduinoPlatform
 * @ingroup arduino_adapters
 * @brief `CW_Platform` implementation using Arduino's blocking `delay()`.
 *
 * Single-method adapter — exists only so the SDK can stay
 * platform-independent. Construct one alongside the other adapters and
 * pass it as the 4th argument of @ref CryptnoxWallet's constructor.
 *
 * @par Example
 * @code
 * ArduinoPlatform       platform;
 * ArduinoCryptoProvider crypto;
 * NullLoggerAdapter     logger;
 * PN532Adapter          nfc(logger, 10);
 * CryptnoxWallet        wallet(nfc, logger, crypto, platform);
 * @endcode
 *
 * @note Blocking — @ref sleep_ms parks the calling task until the timeout
 *       expires. On bare-metal Arduino this is the expected behaviour; on
 *       cooperative-scheduler ports a different platform implementation
 *       (e.g. one that yields) is appropriate.
 */
class ArduinoPlatform : public CW_Platform {
public:
    ArduinoPlatform() = default;
    ~ArduinoPlatform() override = default;

    ArduinoPlatform(const ArduinoPlatform&) = delete;
    ArduinoPlatform& operator=(const ArduinoPlatform&) = delete;

    /**
     * @brief Block for @p ms milliseconds via Arduino's @c delay().
     *
     * @param[in] ms Duration to sleep, in milliseconds.
     */
    void sleep_ms(uint32_t ms) override;
};

#endif // ARDUINOPLATFORM_H
