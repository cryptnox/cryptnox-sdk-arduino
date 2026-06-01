<div align="center">

<img src="https://github.com/user-attachments/assets/6ce54a27-8fb6-48e6-9d1f-da144f43425a"/>

### cryptnox-sdk-arduino

Arduino library for managing Cryptnox Hardware Wallet

</div>

# Connect — Secure Channel + Card Info

Open the Cryptnox Hardware Wallet secure channel from an Arduino UNO R4
and read back the card owner's **name** and **email**. This is the
**safest starting point**: the sketch never sends a PIN, so it cannot
lock the card.

## Requirements

| Component | Details |
|-----------|---------|
| **Hardware Wallet** | Cryptnox Hardware Wallet, initialised (`cryptnox init`) |
| **NFC reader** | PN532 over SPI — see [hardware setup](../../README.md#hardware-setup) |
| **Board** | Arduino UNO R4 Minima or WiFi |
| **SDK** | Installed via `setup.bat` — see [installation](../../README.md#installation) |

No PIN, no seed required.

## Quick start

1. *File → Examples → CryptnoxWallet → Connect*
2. *Tools → Board → Arduino UNO R4 Minima* (or *WiFi*) and *Tools → Port*
3. **Upload**, open the **Serial Monitor** at *115200 baud*
4. Place the card on the PN532 antenna

### Expected output

```
PN532 information
 ├─ Raw firmware: 0x32010607
 ├─ IC Chip: PN532
 ├─ Firmware: 1.6
 └─ Features: MIFARE + ISO-DEP + FeliCa (0x7)
Card connected, secure channel established
Owner name : Alice
Owner email: alice@cryptnox.com
```

## How it works

```
 wallet.begin()                Bring up SPI + reset the PN532
        │
 wallet.connect(session)       SELECT + cert chain verify (secp256r1)
        │                      + ECDH key agreement + mutual auth
        │                      → session.aesKey / macKey / iv populated
        │
 wallet.getCardInfo(session)   Secured APDU (AES-CBC + MAC)
        │                      Decrypt response, parse name & email
        │
 wallet.disconnect(session)    Zero session keys
```

## Step-by-step code

**Wire the adapters together** (declared once at file scope):

```cpp
ArduinoLoggerAdapter   serialAdapter;
PN532Adapter           nfc(serialAdapter, /*SS=*/10U, &SPI);
ArduinoCryptoProvider  cryptoProvider;
ArduinoPlatform        platform;
CryptnoxWallet         wallet(nfc, serialAdapter, cryptoProvider, platform);
```

**Bring up the reader** in `setup()`:

```cpp
serialAdapter.begin(115200);
delay(1000);                          // UNO R4: wait for USB-CDC Serial
SPI.begin();
if (!wallet.begin()) {
    serialAdapter.println(F("PN532 init failed"));
    while (1) {}
}
nfc.printFirmwareVersion();
```

**Open the channel and read the card** in `loop()`:

```cpp
CW_SecureSession session;
if (wallet.connect(session)) {
    serialAdapter.println(F("Card connected, secure channel established"));
    CW_CardInfo info;
    if (wallet.getCardInfo(session, &info)) {
        serialAdapter.print(F("Owner name : "));
        serialAdapter.println(info.name);
        serialAdapter.print(F("Owner email: "));
        serialAdapter.println(info.email);
    } else {
        serialAdapter.println(F("getCardInfo failed (channel error or parse error)"));
    }
} else {
    serialAdapter.println(F("Card not detected or secure channel failed"));
}
wallet.disconnect(session);
```

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `PN532 init failed` | Reader wiring / power / mode switches | Check VCC = 3.3 V, the SCK/MISO/MOSI/SS pinout, and SPI mode switches (`SW0=HIGH, SW1=LOW`) — see [hardware setup](../../README.md#hardware-setup) |
| `Card not detected or secure channel failed` | Card not on the antenna, or card not initialised | Bring the card within ~1 cm of the antenna; run `cryptnox init` if it is a brand-new card |
| `getCardInfo failed (channel error or parse error)` | Card initialised without an owner name/email | Re-run `cryptnox init` and fill in the owner fields |
| `APDU exchange failed!` | NFC link lost mid-exchange | Hold the card steady while the PN532 LED is on |

## License

`cryptnox-sdk-arduino` is dual-licensed:

- **LGPL-3.0** for open-source projects and proprietary projects that comply with LGPL requirements
- **Commercial license** for projects that require a proprietary license without LGPL obligations (see [COMMERCIAL.md](../../COMMERCIAL.md) for details)

For commercial inquiries, contact: contact@cryptnox.com
