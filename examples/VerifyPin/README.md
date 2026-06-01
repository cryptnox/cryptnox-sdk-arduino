<div align="center">

<img src="https://github.com/user-attachments/assets/6ce54a27-8fb6-48e6-9d1f-da144f43425a"/>

### cryptnox-sdk-arduino

Arduino library for managing Cryptnox Hardware Wallet

</div>

# VerifyPin — PIN Verification Over the Secure Channel

Open the Cryptnox Hardware Wallet secure channel and submit a PIN. The
sketch prints `PIN accepted` on success and **halts** the loop on a
wrong PIN so it cannot exhaust the on-card retry counter.

> [!WARNING]
> **Read this before uploading.** Every wrong PIN attempt decrements
> an on-card retry counter (typically 3–5 tries). At zero the PIN is
> permanently blocked; recovery then requires the PUK via
> `cryptnox unblock_pin`. This sketch halts on a wrong PIN — do not
> remove the `while (1)` guard, and verify `DEMO_PIN` matches the
> value used during `cryptnox init` **before** uploading.

## Requirements

| Component | Details |
|-----------|---------|
| **Hardware Wallet** | Cryptnox Hardware Wallet, initialised with a known PIN |
| **NFC reader** | PN532 over SPI — see [hardware setup](../../README.md#hardware-setup) |
| **Board** | Arduino UNO R4 Minima or WiFi |
| **SDK** | Installed via `setup.bat` — see [installation](../../README.md#installation) |

## Quick start

1. **Edit `DEMO_PIN`** in `VerifyPin.ino` to match your card's PIN.
2. *File → Examples → CryptnoxWallet → VerifyPin*
3. *Tools → Board → Arduino UNO R4 Minima* (or *WiFi*) and *Tools → Port*
4. **Upload**, open the **Serial Monitor** at *115200 baud*, place the
   card on the antenna.

### Expected output

**Happy path** (every second):

```
PIN accepted
```

**Wrong PIN** (sketch then halts):

```
Secured APDU: bad SW 0x63C2
PIN rejected — halting to protect retry counter
```

The SDK prints the raw `SW1 SW2` bytes on PIN failure:

- `0x63CN` — wrong PIN, **N** retries remaining
- `0x6983` — PIN already blocked (0 retries)

## How it works

```
 wallet.connect(session)            SELECT + cert verify + ECDH + mutual auth
        │
 wallet.verifyPin(session, ...)     Secured APDU (AES-CBC + MAC) carrying
        │                           the PIN. Card checks it locally and
        │                           replies with SW 0x9000 on success or
        │                           0x63CN on wrong PIN with N retries left.
        │
 wallet.disconnect(session)         Zero session keys
```

`verifyPin` returns `true` **only** when the card replies `0x9000`.

## Step-by-step code

```cpp
#define DEMO_PIN  "000000000"          // must match cryptnox init

CW_SecureSession session;
if (!wallet.connect(session)) {
    serialAdapter.println(F("Card not detected"));
    wallet.disconnect(session);
    delay(1000);
    return;
}

if (!wallet.verifyPin(session,
                      reinterpret_cast<const uint8_t*>(DEMO_PIN),
                      (uint8_t)strlen(DEMO_PIN))) {
    serialAdapter.println(F("PIN rejected — halting to protect retry counter"));
    wallet.disconnect(session);
    while (1) {}                       // CRITICAL: do NOT loop on failure
}

serialAdapter.println(F("PIN accepted"));
wallet.disconnect(session);
```

## Recovering a blocked PIN

If the PIN counter has reached zero, run the
[Cryptnox CLI](https://github.com/cryptnox/cryptnox-cli) on a host
with a PC/SC reader:

```bash
cryptnox unblock_pin
```

The CLI prompts for the **PUK** (set during `cryptnox init`).
Without the PUK the PIN cannot be reset and the on-card signing key
material is unrecoverable.

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `PN532 init failed` | Reader wiring / SPI mode switches | See [hardware setup](../../README.md#hardware-setup) |
| `Card not detected` | Card not on the antenna | Bring the card within ~1 cm of the antenna |
| `bad SW 0x63CN` | Wrong PIN, N retries remaining | Fix `DEMO_PIN`, re-upload |
| `bad SW 0x6983` | PIN blocked (0 retries) | Run `cryptnox unblock_pin` with the PUK |
| `mutuallyAuthenticate failed` | Non-Cryptnox card or seed mismatch | Verify the card was set up with `cryptnox init` |

## License

`cryptnox-sdk-arduino` is dual-licensed:

- **LGPL-3.0** for open-source projects and proprietary projects that comply with LGPL requirements
- **Commercial license** for projects that require a proprietary license without LGPL obligations (see [COMMERCIAL.md](../../COMMERCIAL.md) for details)

For commercial inquiries, contact: contact@cryptnox.com
