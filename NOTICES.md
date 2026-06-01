# Third-party notices

`cryptnox-sdk-arduino` is distributed under a dual LGPL-3.0 / commercial
license (see [README.md](README.md), [LICENSE](LICENSE) and
[COMMERCIAL.md](COMMERCIAL.md)). The first-party source files under
`src/` and the in-repo examples are all © Cryptnox SA, LGPL-3.0-or-later.

The notes below cover (a) the few example files that originate outside
that scope, and (b) the external libraries the SDK depends on at
compile time but does **not** redistribute.

---

## examples/UsdcSigning/keccak256.{cpp,h} — Keccak-f[1600] reference algorithm

The Keccak (SHA-3) sponge construction is a public-domain algorithm
designed by Bertoni, Daemen, Peeters and Van Assche. The file
`examples/UsdcSigning/keccak256.cpp` is a mechanical implementation
written for this project from the FIPS 202 specification; it does not
derive from any specific implementation under copyright. The
round-constant table and rotation offsets are the canonical constants
from the Keccak specification.

---

## Compile-time dependencies (NOT redistributed)

Per `library.properties`, this library depends on the following
Arduino libraries. They are installed independently by the user via
the Arduino Library Manager or the Arduino IDE; nothing in the
`cryptnox-sdk-arduino` source tree includes their source code.

| Library | License |
|---------|---------|
| [micro-ecc](https://github.com/kmackay/micro-ecc) | BSD-2-Clause |
| [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532) | BSD-3-Clause |
| [AESLib](https://github.com/suculent/thinx-aes-lib) | LGPL-3.0 |
| [Crypto](https://rweather.github.io/arduinolibs/crypto.html) | MIT |
| [trng](https://github.com/embarquech/trng) | LGPL-3.0 |
| [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient) | LGPL-2.1-or-later |

Final-product distributions that bundle a compiled artefact (e.g. a
firmware image) must include the licenses of these dependencies in
their notices, per each library's terms.
