<div align="center">

<img src="https://github.com/user-attachments/assets/6ce54a27-8fb6-48e6-9d1f-da144f43425a"/>

### cryptnox-sdk-arduino

Arduino library for managing Cryptnox Hardware Wallet

</div>

# BasicUsage — End-to-End Walkthrough (SPI or I²C)

A single self-contained sketch that exercises the **full Cryptnox
flow** on Arduino: pick SPI or I²C at build time, open the secure
channel, sign a 32-byte hash, wipe the secrets, disconnect. Reads as
a checklist of every step a production sketch will perform.

If you only need **one** of the steps, see the focused examples:

| You want… | See |
|-----------|-----|
| The secure channel + card identity | [Connect](../Connect/README.md) |
| A PIN verification flow | [VerifyPin](../VerifyPin/README.md) |
| A signature without the rest | [Sign](../Sign/README.md) |
| A real Ethereum tx broadcast | [UsdcSigning](../UsdcSigning/README.md) |

## Requirements

| Component | Details |
|-----------|---------|
| **Hardware Wallet** | Cryptnox Hardware Wallet, initialised **and** seeded |
| **NFC reader** | PN532 wired on **SPI** (default) or **I²C** — see [hardware setup](../../README.md#hardware-setup) |
| **Board** | Arduino UNO R4 Minima or WiFi |
| **SDK** | Installed via `setup.bat` — see [installation](../../README.md#installation) |

## Quick start

1. **Pick the bus** at the top of `BasicUsage.ino` — uncomment exactly
   one:

   ```cpp
   #define USE_SPI      // SCK/MISO/MOSI on D13/12/11, SS on D10
   // #define USE_I2C   // SDA on A4, SCL on A5
   ```

2. **Set the PIN** to match `cryptnox init`:

   ```cpp
   #define DEFAULT_PIN  "000000000"
   ```

3. *File → Examples → CryptnoxWallet → BasicUsage*, select the board
   and serial port.

4. **Upload**, open the **Serial Monitor** at *115200 baud*, place the
   card on the PN532 antenna.

### Expected output

```
Card connected and secure channel established
Signing test hash...
Signature received (64 bytes raw r||s)
  R[0..7]: 7C 1F 3A 92 5E 0B 8C D4
  S[0..7]: 12 E0 BC 4F A7 88 09 67
Card processed successfully
```

> [!NOTE]
> The SDK uses `inDataExchange16` internally to handle PN532 extended
> frames, so manufacturer-certificate pages up to ~411 bytes deliver
> correctly on both SPI and I²C.

## How it works

```
 setup():
   Serial / SPI(or Wire) bring-up
   wallet.begin()                          PN532 reset + firmware probe

 loop():
   wallet.connect(session)                 SELECT + cert verify + ECDH
                                           + mutual authentication
   Build CW_SignRequest:
     keyType        = CW_SIGN_CURR_K1
     signatureType  = CW_SIGN_SIG_ECDSA_LOW_S
     pinLessMode    = CW_SIGN_WITH_PIN
     hash[32]       = 0x01 × 32            (test pattern)
     pin[]          = DEFAULT_PIN
   wallet.sign(req)                        SIGN APDU under the channel
   secure_wipe(hash, signature)            Zero local copies
   wallet.disconnect(session)              Zero session keys
   delay(1000)
```

## Step-by-step code

**1. Interface selection** — exactly one of:

```cpp
#define USE_SPI
// #define USE_I2C
```

On `USE_SPI` the sketch instantiates:

```cpp
PN532Adapter nfc(serialAdapter, /*SS=*/10U, &SPI);
```

On `USE_I2C` it wires the optional IRQ/RST pins (D2/D3). These are
typically **not connected** on the Keyestudio Ks0259 shield, and the
upstream Adafruit library polls the IRQ pin in software regardless,
so the sketch works with these pins floating on that shield:

```cpp
PN532Adapter nfc(serialAdapter, /*IRQ=*/2U, /*RST=*/3U, &Wire);
```

**2. Bring up the bus and reader:**

```cpp
void setup() {
    serialAdapter.begin(115200);
    delay(1000);                          // UNO R4: wait for USB-CDC Serial
#if defined(USE_SPI)
    SPI.begin();
#elif defined(USE_I2C)
    Wire.begin();
#endif
    if (!wallet.begin()) {
        serialAdapter.println(F("PN532 init failed"));
        while (1) {}
    }
}
```

**3. Open the channel, sign, and wipe:**

```cpp
CW_SecureSession session;
if (wallet.connect(session)) {
    uint8_t testHash[CW_HASH_SIZE];
    memset(testHash, 0x01, sizeof(testHash));   // replace with SHA-256(tx)

    CW_SignRequest req(session, CW_SIGN_CURR_K1,
                       CW_SIGN_SIG_ECDSA_LOW_S, CW_SIGN_WITH_PIN);
    req.hash       = testHash;
    req.hashLength = sizeof(testHash);
    CW_Utils::safe_memcpy(req.pin, sizeof(req.pin),
                          reinterpret_cast<const uint8_t*>(DEFAULT_PIN),
                          DEFAULT_PIN_LEN);

    CW_SignResult sig = wallet.sign(req);
    // sig.signature = r[32] || s[32], sig.errorCode == CW_OK on success

    CW_Utils::secure_wipe(testHash, sizeof(testHash));
    CW_Utils::secure_wipe(sig.signature, sizeof(sig.signature));
}
wallet.disconnect(session);
```

## Hardening for production

This sketch is a demo. Before shipping firmware to end-users:

- **Silence Serial.** Replace `ArduinoLoggerAdapter serialAdapter;`
  with `NullLoggerAdapter serialAdapter;` and drop the
  `serialAdapter.begin()` call. The null adapter is a no-op (zero
  code-size / runtime cost) and stops the SDK from emitting APDU and
  key material on USB-CDC.
- **Move the PIN off flash.** A hardcoded `DEFAULT_PIN` lives in
  `.rodata` and is recoverable via SWD/JTAG. In production read the
  PIN at runtime (keypad, BLE prompt, companion chip) and call
  `CW_Utils::secure_wipe` on the buffer after `wallet.sign()`.
- **Guard the PIN retry counter.** Apply the halt-on-wrong-PIN
  pattern from [VerifyPin](../VerifyPin/README.md) /
  [Sign](../Sign/README.md) so the loop cannot exhaust the on-card
  counter.

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `Please define USE_SPI or USE_I2C…` (build error) | Neither macro defined | Uncomment exactly one of `#define USE_SPI` / `#define USE_I2C` |
| `PN532 init failed` | Reader wiring / bus mode switches | Check VCC = 3.3 V, the switches and the configured pins — see [hardware setup](../../README.md#hardware-setup) |
| `Sign failed, errorCode: 0x81` | No seed on the card | `cryptnox seed generate` |
| `Sign failed, errorCode: 0x82` | Wrong PIN | Edit `DEFAULT_PIN`, re-upload — see [VerifyPin](../VerifyPin/README.md) |
| Sketch overflows flash | Memory-optimisation script not run | Re-run `setup.bat` — it calls `scripts/enable_memory_optimization.bat` |

## License

`cryptnox-sdk-arduino` is dual-licensed:

- **LGPL-3.0** for open-source projects and proprietary projects that comply with LGPL requirements
- **Commercial license** for projects that require a proprietary license without LGPL obligations (see [COMMERCIAL.md](../../COMMERCIAL.md) for details)

For commercial inquiries, contact: contact@cryptnox.com
