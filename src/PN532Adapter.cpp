/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file PN532Adapter.cpp
 * @brief Implementation of the PN532 transport adapter.
 *
 * Forwards each @ref CW_NfcTransport call to the underlying
 * `Adafruit_PN532` instance, plus:
 *   - upfront APDU-length bound check (HIGH-05),
 *   - optional response hex-dump when @c CW_DEBUG_LOGGING is set,
 *   - firmware-version pretty-printer used by @ref CryptnoxWallet::begin().
 *
 * Doxygen documentation lives on the declarations in @ref PN532Adapter.h.
 */

#include "PN532Adapter.h"

// cppcheck-suppress misra-c2012-12.3 -- C++: member initializer-list commas are not the comma operator
PN532Adapter::PN532Adapter(CW_Logger& logger, uint8_t ssPin, SPIClass *theSPI)
    : _logger(&logger), _interface(PN532Interface::SPI_HARDWARE),
      _nfc(new Adafruit_PN532(ssPin, theSPI)) {}

// cppcheck-suppress misra-c2012-12.3 -- C++: member initializer-list commas are not the comma operator
PN532Adapter::PN532Adapter(CW_Logger& logger, uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss)
    : _logger(&logger), _interface(PN532Interface::SPI_SOFTWARE),
      _nfc(new Adafruit_PN532(clk, miso, mosi, ss)) {}

// cppcheck-suppress misra-c2012-12.3 -- C++: member initializer-list commas are not the comma operator
PN532Adapter::PN532Adapter(CW_Logger& logger, uint8_t irqPin, uint8_t resetPin, TwoWire *wire)
    : _logger(&logger), _interface(PN532Interface::I2C),
      _nfc(new Adafruit_PN532(irqPin, resetPin, wire)) {}

// cppcheck-suppress misra-c2012-12.3 -- C++: member initializer-list commas are not the comma operator
PN532Adapter::PN532Adapter(CW_Logger& logger, uint8_t resetPin, HardwareSerial *uartSerial)
    : _logger(&logger), _interface(PN532Interface::UART),
      _nfc(new Adafruit_PN532(resetPin, uartSerial)) {}

PN532Adapter::~PN532Adapter() {
    if (_nfc) {
        delete _nfc;
        _nfc = NULL;
    }
}

bool PN532Adapter::begin() {
    _nfc->begin();
    return _nfc->getFirmwareVersion() != 0;
}

bool PN532Adapter::sendAPDU(const uint8_t* apdu, uint8_t apduLen,
                            uint8_t* response, uint8_t& responseLen) {
    /* HIGH-05: the uint8_t apduLen already constrains the APDU to the
     * PN532's ISO-DEP 255-byte limit at the type level; no runtime check
     * needed. */

    /* Adafruit_PN532::inDataExchange takes a uint8_t* for the response
     * length (in = capacity, out = actual), matching our uint8_t& on
     * the CW_NfcTransport contract — pass it through directly. */
    bool success = _nfc->inDataExchange(const_cast<uint8_t*>(apdu), apduLen,
                                        response, &responseLen);
    if (!success) {
        _logger->println(F("APDU exchange failed!"));
        return false;
    }

#if CW_DEBUG_LOGGING
    _logger->print(F("APDU response ("));
    _logger->print(responseLen);
    _logger->println(F(" bytes):"));

    for (uint8_t i = 0; i < responseLen; i++) {
        _logger->print(F("0x"));
        if (response[i] < 16) { _logger->print(F("0")); }
        _logger->print(response[i], HEX);
        _logger->print(F(" "));
        if (((i + 1) % 16 == 0) && ((i + 1) != responseLen)) { _logger->println(); }
    }
    _logger->println();
#endif /* CW_DEBUG_LOGGING */

    return true;
}

bool PN532Adapter::sendAPDULarge(const uint8_t* apdu, uint8_t apduLen,
                                 uint8_t* response, uint16_t& responseLen) {
    /* Adafruit_PN532::inDataExchange16 is provided by the
     * Adafruit_PN532_extended_frame.patch. It handles both PN532 normal
     * frames (1-byte LEN, <=255-byte payload) and PN532 extended frames
     * (0xFF 0xFF + 16-bit LEN, up to ~436-byte payload). The Cryptnox
     * card's GET_MANUFACTURER_CERTIFICATE per-page answer exceeds the
     * normal-frame ceiling on cards whose cert is ~411 bytes, so
     * delegating to the default uint8_t sendAPDU would lose the data. */
    bool success = _nfc->inDataExchange16(const_cast<uint8_t*>(apdu), apduLen,
                                          response, &responseLen);
    if (!success) {
        _logger->println(F("APDU exchange (large) failed!"));
        return false;
    }

#if CW_DEBUG_LOGGING
    _logger->print(F("APDU response ("));
    _logger->print(responseLen);
    _logger->println(F(" bytes, large):"));

    for (uint16_t i = 0; i < responseLen; i++) {
        _logger->print(F("0x"));
        if (response[i] < 16) { _logger->print(F("0")); }
        _logger->print(response[i], HEX);
        _logger->print(F(" "));
        if (((i + 1) % 16 == 0) && ((i + 1) != responseLen)) { _logger->println(); }
    }
    _logger->println();
#endif /* CW_DEBUG_LOGGING */

    return true;
}

bool PN532Adapter::inListPassiveTarget() {
    return _nfc->inListPassiveTarget();
}

void PN532Adapter::resetReader() {
    _nfc->SAMConfig();
}

bool PN532Adapter::printFirmwareVersion() {
    uint32_t versionData = _nfc->getFirmwareVersion();

    if (versionData == 0) {
        _logger->println(F("PN532 not found!"));
        return false;
    }

    uint8_t ic       = (versionData >> 24U) & 0xFFU;
    uint8_t verMajor = (versionData >> 16U) & 0xFFU;
    uint8_t verMinor = (versionData >>  8U) & 0xFFU;
    uint8_t flags    =  versionData         & 0xFFU;
    bool first       = true;

    _logger->println(F("PN532 information"));
    _logger->print(F(" ├─ Raw firmware: 0x"));
    _logger->println(versionData, HEX);

    _logger->print(F(" ├─ IC Chip: "));
    _logger->println((ic == 0x32U) ? F("PN532") : F("Unknown"));

    _logger->print(F(" ├─ Firmware: "));
    _logger->print(verMajor);
    _logger->print(F("."));
    _logger->println(verMinor);

    _logger->print(F(" └─ Features: "));
    if ((flags & 0x01U) != 0U) { _logger->print(F("MIFARE")); first = false; }
    if ((flags & 0x02U) != 0U) {
        if (!first) { _logger->print(F(" + ")); }
        _logger->print(F("ISO-DEP")); first = false;
    }
    if ((flags & 0x04U) != 0U) {
        if (!first) { _logger->print(F(" + ")); }
        _logger->print(F("FeliCa")); first = false;
    }
    if (first) { _logger->print(F("Unknown")); }

    _logger->print(F(" (0x"));
    _logger->print(flags, HEX);
    _logger->println(F(")"));

    _nfc->SAMConfig();
    return true;
}
