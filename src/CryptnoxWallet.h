/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file CryptnoxWallet.h
 * @brief Umbrella include and module-group anchor for the CryptnoxWallet Arduino library.
 *
 * Including this single header pulls in the platform-independent core SDK
 * (the `cryptnox-sdk-cpp` submodule) together with all Arduino-specific
 * concrete adapters (PN532 transport, AESLib/SHA512/micro-ecc crypto provider,
 * Serial logger, null logger). Application sketches should not include the
 * individual headers directly — this umbrella keeps the include surface
 * stable across Arduino IDE versions and library-manager installs.
 *
 * | Header pulled in                                  | Role |
 * |---------------------------------------------------|------|
 * | `cryptnox-sdk-cpp/CW_Defs.h`                      | Shared constants, `CW_SecureSession` struct, group definitions |
 * | `cryptnox-sdk-cpp/CW_NfcTransport.h`              | Abstract NFC transport interface |
 * | `cryptnox-sdk-cpp/CW_Logger.h`                    | Abstract logging interface |
 * | `cryptnox-sdk-cpp/CW_CryptoProvider.h`            | Abstract crypto interface (SHA, AES-CBC, ECDH, RNG, ECDSA verify) |
 * | `cryptnox-sdk-cpp/CW_Platform.h`                  | Abstract platform interface (`sleep_ms`) |
 * | `cryptnox-sdk-cpp/CW_SecureChannel.h`             | Secure channel protocol implementation |
 * | `cryptnox-sdk-cpp/CW_Utils.h`                     | Secure compare, secure wipe, bounded memcpy |
 * | `cryptnox-sdk-cpp/CryptnoxWallet.h`               | High-level card API |
 * | `ArduinoLoggerAdapter.h`                          | Concrete `CW_Logger` over `HardwareSerial` (dev/debug) |
 * | `NullLoggerAdapter.h`                             | Silent no-op `CW_Logger` (production — see LOW-03) |
 * | `ArduinoCryptoProvider.h`                         | Concrete `CW_CryptoProvider` (AESLib / SHA-512 / micro-ecc / RA4M1 TRNG) |
 * | `ArduinoPlatform.h`                               | Concrete `CW_Platform` over Arduino's blocking `delay()` |
 * | `PN532Adapter.h`                                  | Concrete `CW_NfcTransport` over Adafruit_PN532 |
 *
 * @par Minimal sketch
 * @code
 * #include <CryptnoxWallet.h>
 *
 * NullLoggerAdapter     logger;             // silent in production
 * ArduinoCryptoProvider crypto;             // AES + SHA + ECDH + TRNG
 * PN532Adapter          nfc(logger, 10);    // SPI on SS=10
 * ArduinoPlatform       platform;
 * CryptnoxWallet        wallet(nfc, logger, crypto, platform);
 *
 * void setup() {
 *     wallet.begin();
 * }
 * @endcode
 *
 * @see CryptnoxWallet
 * @see CW_NfcTransport
 * @see CW_CryptoProvider
 * @see CW_Logger
 */

/**
 * @defgroup arduino_adapters Arduino concrete adapters
 * @brief Concrete `CW_NfcTransport` / `CW_CryptoProvider` / `CW_Logger` / `CW_Platform` implementations for the Arduino UNO R4 (RA4M1).
 *
 * Each class in this group implements one of the abstract interfaces declared
 * in @ref adapters using libraries available through the Arduino Library
 * Manager:
 *
 * | Adapter                  | Interface           | Backing library / hardware |
 * |--------------------------|---------------------|----------------------------|
 * | @ref PN532Adapter          | `CW_NfcTransport`   | Adafruit_PN532 (SPI / I2C / UART) |
 * | @ref ArduinoCryptoProvider | `CW_CryptoProvider` | AESLib + SHA512/SHA256 + micro-ecc + RA4M1 TRNG |
 * | @ref ArduinoLoggerAdapter  | `CW_Logger`         | Arduino `HardwareSerial` (development builds) |
 * | @ref NullLoggerAdapter     | `CW_Logger`         | No-op (production builds — keeps APDU/key material off the UART) |
 * | @ref ArduinoPlatform       | `CW_Platform`       | Arduino blocking `delay()` |
 */
#pragma once

#include "cryptnox-sdk-cpp/CW_Defs.h"
#include "cryptnox-sdk-cpp/CW_NfcTransport.h"
#include "cryptnox-sdk-cpp/CW_Logger.h"
#include "cryptnox-sdk-cpp/CW_CryptoProvider.h"
#include "cryptnox-sdk-cpp/CW_Platform.h"
#include "cryptnox-sdk-cpp/CW_SecureChannel.h"
#include "cryptnox-sdk-cpp/CW_Utils.h"
#include "cryptnox-sdk-cpp/CryptnoxWallet.h"
#include "ArduinoLoggerAdapter.h"
#include "NullLoggerAdapter.h"
#include "ArduinoCryptoProvider.h"
#include "ArduinoPlatform.h"
#include "PN532Adapter.h"
