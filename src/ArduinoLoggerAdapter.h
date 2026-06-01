/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file ArduinoLoggerAdapter.h
 * @brief Concrete `CW_Logger` over `HardwareSerial` for development builds.
 *
 * Declares @ref ArduinoLoggerAdapter, a thin pass-through adapter that
 * sends every log call to an Arduino `HardwareSerial` (defaults to the
 * primary `Serial`). Intended for development and debugging.
 *
 * @see CW_Logger
 * @see NullLoggerAdapter — production-time replacement that silences output.
 */

#ifndef ARDUINOLOGGERADAPTER_H
#define ARDUINOLOGGERADAPTER_H

#include <Arduino.h>
#include "cryptnox-sdk-cpp/CW_Logger.h"

/**
 * @class ArduinoLoggerAdapter
 * @ingroup arduino_adapters
 * @brief `CW_Logger` implementation wrapping Arduino's `HardwareSerial`.
 *
 * Lets @ref CryptnoxWallet and @ref CW_SecureChannel emit debug traces
 * through the standard Arduino `Serial` API. By default the adapter wraps
 * the primary `Serial` object, but any `HardwareSerial` instance can be
 * injected — useful on the UNO R4 where `Serial1` is exposed on a separate
 * connector.
 *
 * @par Example
 * @code
 * ArduinoLoggerAdapter logger;             // wraps Serial
 * ArduinoLoggerAdapter logger1(&Serial1);  // wraps Serial1
 * logger.begin(115200);
 * @endcode
 *
 * @warning In production firmware, switch to @ref NullLoggerAdapter — when
 *          `CW_DEBUG_LOGGING` is set the SDK emits raw APDUs and (briefly)
 *          PIN-related debug traces on the serial port (LOW-03). The
 *          NullLoggerAdapter route guarantees nothing reaches the UART
 *          regardless of compile-time flags.
 */
class ArduinoLoggerAdapter : public CW_Logger {
public:
    /**
     * @brief Construct an adapter that writes to the primary `Serial`.
     *
     * Equivalent to @c ArduinoLoggerAdapter(&Serial). The underlying serial
     * port is not opened — call @ref begin() once early in @c setup() with
     * the desired baud rate before any log call.
     */
    ArduinoLoggerAdapter();

    /**
     * @brief Construct an adapter targeting a specific `HardwareSerial`.
     *
     * @param[in] serial Pointer to the HardwareSerial to wrap (e.g. @c &Serial1).
     *                   Must outlive the adapter; the adapter does not own
     *                   the underlying object.
     */
    explicit ArduinoLoggerAdapter(HardwareSerial* serial);

    ~ArduinoLoggerAdapter() override = default;

    ArduinoLoggerAdapter(const ArduinoLoggerAdapter&) = delete;
    ArduinoLoggerAdapter& operator=(const ArduinoLoggerAdapter&) = delete;

    /** @name CW_Logger interface */
    ///@{

    /**
     * @brief Open the wrapped HardwareSerial at the given baud rate.
     *
     * Thin wrapper over @c HardwareSerial::begin(). Must be called from
     * the sketch's @c setup() before any log call.
     *
     * @param[in] baudRate Baud rate to open the port at (default 115200).
     * @return Always @c true — the underlying Arduino API has no failure
     *         channel.
     */
    bool begin(unsigned long baudRate = 115200UL) override;

    void print(const __FlashStringHelper* str) override; ///< Forwards to @c HardwareSerial::print(F-string).
    void print(const char* str) override;                ///< Forwards to @c HardwareSerial::print(const char*).
    void print(char c) override;                         ///< Forwards to @c HardwareSerial::print(char).
    void print(uint8_t value, int base = DEC) override;  ///< Forwards to @c HardwareSerial::print(uint8_t, base).
    void print(uint16_t value, int base = DEC) override; ///< Forwards to @c HardwareSerial::print(uint16_t, base).
    void print(uint32_t value, int base = DEC) override; ///< Forwards to @c HardwareSerial::print(uint32_t, base).
    void print(int value, int base = DEC) override;     ///< Forwards to @c HardwareSerial::print(int, base).

    void println() override;                               ///< Emits a CR/LF.
    void println(const __FlashStringHelper* str) override; ///< Forwards then CR/LF.
    void println(const char* str) override;                ///< Forwards then CR/LF.
    void println(char c) override;                         ///< Forwards then CR/LF.
    void println(uint8_t value, int base = DEC) override;  ///< Forwards then CR/LF.
    void println(uint16_t value, int base = DEC) override; ///< Forwards then CR/LF.
    void println(uint32_t value, int base = DEC) override; ///< Forwards then CR/LF.
    void println(int value, int base = DEC) override;      ///< Forwards then CR/LF.
    ///@}

private:
    HardwareSerial* _serial; ///< Non-owning pointer to the wrapped HardwareSerial.
};

#endif // ARDUINOLOGGERADAPTER_H
