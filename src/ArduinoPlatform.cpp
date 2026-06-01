/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

/**
 * @file ArduinoPlatform.cpp
 * @brief Implementation of the Arduino blocking-delay platform adapter.
 */

#include "ArduinoPlatform.h"

/**
 * @brief Forward to Arduino's @c delay().
 */
void ArduinoPlatform::sleep_ms(uint32_t ms) {
    delay(ms);
}
