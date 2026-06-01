/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file PN532Adapter.h
 * @brief Concrete `CW_NfcTransport` over Adafruit_PN532 (SPI / I2C / UART).
 *
 * Declares @ref PN532Adapter, the Arduino-side concrete transport that the
 * host integration injects into @ref CryptnoxWallet. Wraps the four wiring
 * variants the [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532)
 * library supports — hardware SPI, software (bit-banged) SPI, I2C, and
 * UART — behind the single platform-independent @ref CW_NfcTransport
 * interface so the higher layers never need to know how the reader is
 * actually wired.
 *
 * @see CW_NfcTransport
 * @see CryptnoxWallet
 */

#ifndef PN532ADAPTER_H
#define PN532ADAPTER_H

#include <Arduino.h>
#include <Adafruit_PN532.h>
#include "cryptnox-sdk-cpp/CW_NfcTransport.h"
#include "cryptnox-sdk-cpp/CW_Logger.h"
#include "cryptnox-sdk-cpp/CW_Defs.h"

/**
 * @enum PN532Interface
 * @ingroup arduino_adapters
 * @brief Physical wiring used to reach the PN532 reader.
 *
 * Set internally by the constructor matching the wiring the caller used.
 * Exposed for diagnostics — application code does not normally need to
 * read it.
 *
 * | Mode             | Pros                                   | Cons                              |
 * |------------------|----------------------------------------|-----------------------------------|
 * | @c SPI_HARDWARE  | Fastest, uses the MCU SPI peripheral.  | Pins are fixed by the SPI block.  |
 * | @c SPI_SOFTWARE  | Any GPIO; useful when SPI is in use.   | Slower, more CPU.                 |
 * | @c I2C           | 2 wires + IRQ + RESET.                 | Slower than SPI; needs pull-ups.  |
 * | @c UART          | 2 wires + RESET; long traces tolerated.| Slowest; consumes a HardwareSerial. |
 */
enum class PN532Interface {
    SPI_HARDWARE, ///< Hardware SPI via an @c SPIClass instance.
    SPI_SOFTWARE, ///< Bit-banged SPI on four arbitrary GPIOs.
    I2C,          ///< I2C via a @c TwoWire instance.
    UART          ///< UART via a @c HardwareSerial instance.
};

/**
 * @class PN532Adapter
 * @ingroup arduino_adapters
 * @brief `CW_NfcTransport` implementation over the Adafruit_PN532 driver.
 *
 * Concrete transport the host integration constructs once per reader and
 * passes by reference to @ref CryptnoxWallet. The class owns the underlying
 * @c Adafruit_PN532 instance (heap-allocated by the constructor matching
 * the wiring, released by the destructor) — callers do not need to manage
 * its lifetime.
 *
 * Forwards each @ref CW_NfcTransport call to the equivalent Adafruit_PN532
 * method, plus a bounds check on APDU length (HIGH-05) and an optional
 * hex-dump of the response when @c CW_DEBUG_LOGGING is set.
 *
 * @par Example — SPI hardware (Arduino UNO R4 default SPI pins)
 * @code
 * ArduinoLoggerAdapter logger;
 * PN532Adapter         nfc(logger, 10);            // SS on D10
 * nfc.begin();
 * @endcode
 *
 * @par Example — I2C (PN532 with IRQ + RESET)
 * @code
 * PN532Adapter nfc(logger, 2, 3);                  // IRQ=D2, RESET=D3
 * @endcode
 *
 * @note Non-copyable: the wrapped Adafruit_PN532 is owned uniquely.
 * @warning The PN532 module has a hard 255-byte limit on ISO-DEP APDUs.
 *          @ref sendAPDU rejects anything longer (HIGH-05) — chunking
 *          must be done in the protocol layer.
 */
class PN532Adapter : public CW_NfcTransport {
public:
    /**
     * @brief Construct an adapter wired over **hardware SPI**.
     *
     * Uses the MCU's SPI peripheral; SCK/MOSI/MISO are fixed by the SPI
     * block (D13/D11/D12 on the UNO R4). Only the slave-select pin is
     * configurable.
     *
     * @param[in] logger Logger sink for debug and error output.
     * @param[in] ssPin  GPIO connected to the PN532 SS / NSS pin.
     * @param[in] theSPI Optional SPI instance (defaults to the global @c SPI).
     */
    PN532Adapter(CW_Logger& logger, uint8_t ssPin, SPIClass *theSPI = &SPI);

    /**
     * @brief Construct an adapter wired over **bit-banged software SPI**.
     *
     * Use this when the hardware SPI peripheral is already taken by another
     * device. Any four GPIOs work; throughput is significantly lower than
     * hardware SPI.
     *
     * @param[in] logger Logger sink.
     * @param[in] clk    GPIO for SPI clock.
     * @param[in] miso   GPIO for MISO (PN532 → MCU).
     * @param[in] mosi   GPIO for MOSI (MCU → PN532).
     * @param[in] ss     GPIO for slave-select.
     */
    PN532Adapter(CW_Logger& logger, uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss);

    /**
     * @brief Construct an adapter wired over **I2C**.
     *
     * Requires the PN532 SEL0/SEL1 jumpers to be set to I2C and external
     * pull-ups on SDA/SCL (the PN532 breakouts from Adafruit include them).
     *
     * @param[in] logger   Logger sink.
     * @param[in] irqPin   GPIO connected to the PN532 IRQ line.
     * @param[in] resetPin GPIO connected to the PN532 RSTPDN line.
     * @param[in] wire     Optional `TwoWire` instance (defaults to the global @c Wire).
     */
    PN532Adapter(CW_Logger& logger, uint8_t irqPin, uint8_t resetPin, TwoWire *wire = &Wire);

    /**
     * @brief Construct an adapter wired over **UART**.
     *
     * The PN532 must be configured at 115200 baud by its SEL jumpers. UART
     * is the slowest mode but tolerates long cable runs better than SPI/I2C.
     *
     * @param[in] logger     Logger sink.
     * @param[in] resetPin   GPIO connected to the PN532 RSTPDN line.
     * @param[in] uartSerial HardwareSerial connected to the PN532 TX/RX lines.
     */
    PN532Adapter(CW_Logger& logger, uint8_t resetPin, HardwareSerial *uartSerial);

    /** @brief Release the heap-allocated `Adafruit_PN532` instance. */
    ~PN532Adapter() override;

    PN532Adapter(const PN532Adapter&) = delete;
    PN532Adapter& operator=(const PN532Adapter&) = delete;

    /** @name CW_NfcTransport interface */
    ///@{

    /**
     * @brief Initialise the PN532 and probe its firmware version.
     *
     * Calls @c Adafruit_PN532::begin() then issues @c getFirmwareVersion()
     * as a liveness check.
     *
     * @return @c true if the reader responded to @c getFirmwareVersion(),
     *         @c false on a wiring fault or a dead module.
     */
    bool begin() override;

    /**
     * @brief Exchange one ISO-DEP APDU with the currently selected card.
     *
     * Wraps @c Adafruit_PN532::inDataExchange() and adapts its semantics to
     * the @ref CW_NfcTransport contract: @p responseLen is in/out
     * (capacity in, actual length out). The @c uint8_t @p apduLen already
     * enforces the PN532's 255-byte ISO-DEP frame limit at the type level
     * (HIGH-05).
     *
     * When @c CW_DEBUG_LOGGING is set, dumps the response as a hex grid to
     * the logger.
     *
     * @param[in]     apdu        APDU bytes (≤ 255).
     * @param[in]     apduLen     Length of @p apdu in bytes.
     * @param[out]    response    Response buffer (caller-allocated).
     * @param[in,out] responseLen In: capacity of @p response.
     *                            Out: number of bytes actually written.
     * @return @c true on a successful exchange, @c false on a PN532 / card
     *         communication failure.
     */
    bool sendAPDU(const uint8_t* apdu, uint8_t apduLen,
                  uint8_t* response, uint8_t& responseLen) override;

    /**
     * @brief APDU exchange that can return more than 255 bytes.
     *
     * Uses Adafruit_PN532's @c inDataExchange16() (added by the
     * @c Adafruit_PN532_extended_frame.patch) which parses both PN532
     * normal and extended frame formats — required by
     * @c GET_MANUFACTURER_CERTIFICATE pages that exceed the 255-byte
     * normal-frame ceiling on cards whose cert is ~411 bytes.
     *
     * @param[in]     apdu        APDU bytes (≤ 255).
     * @param[in]     apduLen     Length of @p apdu in bytes.
     * @param[out]    response    Response buffer (caller-allocated).
     * @param[in,out] responseLen In: capacity of @p response (up to ~436).
     *                            Out: number of bytes actually written.
     * @return @c true on a successful exchange, @c false on a PN532 / card
     *         communication failure or frame parse error.
     */
    bool sendAPDULarge(const uint8_t* apdu, uint8_t apduLen,
                       uint8_t* response, uint16_t& responseLen) override;

    /**
     * @brief Poll for a card in the field.
     *
     * Single shot — returns immediately after one polling cycle. Callers
     * typically invoke this in a loop with a small @c delay().
     *
     * @return @c true if a card was selected, @c false otherwise.
     */
    bool inListPassiveTarget() override;

    /**
     * @brief Reset the PN532 into a clean idle state.
     *
     * Issues @c SAMConfig() which both clears any pending card session and
     * arms the Secure Access Module. Called by @ref CryptnoxWallet between
     * retries so a stuck reader can recover without a hard reset.
     */
    void resetReader() override;

    /**
     * @brief Pretty-print the PN532 firmware version + supported features.
     *
     * Decodes the raw firmware-version word into IC type (@c 0x32 = PN532),
     * major/minor revision, and a feature flag set (MIFARE / ISO-DEP /
     * FeliCa) and emits a tree-style summary to the logger.
     *
     * @return @c true if the PN532 responded, @c false otherwise.
     */
    bool printFirmwareVersion() override;
    ///@}

private:
    CW_Logger*      _logger;    ///< Logger reference (non-owning).
    PN532Interface  _interface; ///< Wiring variant the active constructor selected.
    Adafruit_PN532* _nfc;       ///< Owned Adafruit_PN532 driver instance.
};

#endif // PN532ADAPTER_H
