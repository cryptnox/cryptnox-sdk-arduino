/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file NullLoggerAdapter.h
 * @brief Silent `CW_Logger` for production firmware.
 *
 * Declares @ref NullLoggerAdapter, a no-op logger that drops every call.
 * Use this in shipping firmware instead of @ref ArduinoLoggerAdapter so
 * the SDK's debug traces never reach the UART.
 *
 * @see CW_Logger
 * @see ArduinoLoggerAdapter — development-time counterpart.
 */

#ifndef NULLLOGGERADAPTER_H
#define NULLLOGGERADAPTER_H

#include <Arduino.h>
#include "cryptnox-sdk-cpp/CW_Logger.h"

/**
 * @class NullLoggerAdapter
 * @ingroup arduino_adapters
 * @brief No-op `CW_Logger` — guarantees nothing reaches the serial port.
 *
 * Every `CW_Logger` method is an empty inline override, so:
 *
 *  - The compiler dead-code-eliminates the formatting work at the call
 *    site (no string format, no hex conversion, no `Serial` write).
 *  - Even with @c CW_DEBUG_LOGGING enabled in the build, no APDU contents,
 *    no session-key fragments, no PIN-handling traces leave the chip
 *    through the UART (audit findings LOW-03 / MED-02).
 *  - Required for any deployment where a physically accessible UART
 *    must not become a side-channel.
 *
 * @par Example — production wiring
 * @code
 * NullLoggerAdapter     logger;             // silent
 * ArduinoCryptoProvider crypto;
 * PN532Adapter          nfc(logger, 10);
 * CryptnoxWallet        wallet(nfc, logger, crypto, platform);
 * @endcode
 *
 * @note Drop-in interchangeable with @ref ArduinoLoggerAdapter — flip the
 *       one declaration line to switch between dev and production builds.
 */
class NullLoggerAdapter : public CW_Logger {
public:
    NullLoggerAdapter() = default;
    ~NullLoggerAdapter() override = default;

    NullLoggerAdapter(const NullLoggerAdapter&) = delete;
    NullLoggerAdapter& operator=(const NullLoggerAdapter&) = delete;

    /** @brief No-op begin(); returns true to match the interface contract. */
    bool begin(unsigned long /*baudRate*/ = 115200UL) override { return true; }

    void print(const __FlashStringHelper* /*str*/) override {}        ///< No-op.
    void print(const char* /*str*/) override {}                       ///< No-op.
    void print(char /*c*/) override {}                                ///< No-op.
    void print(uint8_t /*value*/, int /*base*/ = DEC) override {}     ///< No-op.
    void print(uint16_t /*value*/, int /*base*/ = DEC) override {}    ///< No-op.
    void print(uint32_t /*value*/, int /*base*/ = DEC) override {}    ///< No-op.
    void print(int /*value*/, int /*base*/ = DEC) override {}         ///< No-op.

    void println() override {}                                          ///< No-op.
    void println(const __FlashStringHelper* /*str*/) override {}        ///< No-op.
    void println(const char* /*str*/) override {}                       ///< No-op.
    void println(char /*c*/) override {}                                ///< No-op.
    void println(uint8_t /*value*/, int /*base*/ = DEC) override {}     ///< No-op.
    void println(uint16_t /*value*/, int /*base*/ = DEC) override {}    ///< No-op.
    void println(uint32_t /*value*/, int /*base*/ = DEC) override {}    ///< No-op.
    void println(int /*value*/, int /*base*/ = DEC) override {}         ///< No-op.
};

#endif // NULLLOGGERADAPTER_H
