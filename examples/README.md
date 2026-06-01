<div align="center">

<img src="https://github.com/user-attachments/assets/6ce54a27-8fb6-48e6-9d1f-da144f43425a"/>

### cryptnox-sdk-arduino

Arduino library for managing Cryptnox Hardware Wallet

</div>

# Examples

Standalone Arduino sketches that exercise the Cryptnox Hardware Wallet
over NFC (PN532). Each sketch ships with its own focused README so a
reader landing on a single example gets everything needed end-to-end.

## Prerequisites

| Component | Details |
|-----------|---------|
| **Hardware Wallet** | Cryptnox Hardware Wallet (firmware ≥ v1.6.0), initialised with a PIN — and a seed loaded for the signing examples |
| **NFC reader** | [PN532 NFC module](https://www.nxp.com/products/PN532) wired over **SPI** (default) or **I²C** — see [hardware setup](../README.md#hardware-setup) |
| **Board** | [Arduino UNO R4 Minima](https://store.arduino.cc/products/uno-r4-minima) or [Arduino UNO R4 WiFi](https://store.arduino.cc/products/uno-r4-wifi) (the **WiFi** variant is required for [UsdcSigning](UsdcSigning/README.md)) |
| **IDE** | [Arduino IDE 2.x](https://www.arduino.cc/en/software) with the **Arduino UNO R4 Boards** core installed |
| **SDK** | Installed via `setup.bat` from the repository root — see [installation](../README.md#installation) |

Provision a card from a host with a PC/SC reader and the
[Cryptnox CLI](https://github.com/cryptnox/cryptnox-cli):

```bash
cryptnox init           # sets the PIN + PUK
cryptnox seed generate        # generates a BIP39 seed (required for signing)
```

## Available examples

| Example | What it does |
|---------|--------------|
| [Connect](Connect/README.md) | Opens the secure channel and reads back the card owner's name & email. **Safest first sketch** — no PIN, no signing, can't lock the card. |
| [VerifyPin](VerifyPin/README.md) | Opens the secure channel and submits a PIN. Halts on a wrong PIN to protect the on-card retry counter. |
| [Sign](Sign/README.md) | Signs a 32-byte hash with the card's secp256k1 key. Returns the raw `r ‖ s` signature ready to broadcast. |
| [BasicUsage](BasicUsage/README.md) | End-to-end walkthrough in one sketch: pick SPI **or** I²C, open the channel, sign a hash. Good reference for production wiring. |
| [UsdcSigning](UsdcSigning/README.md) | Real-world flow on UNO R4 WiFi: build an EIP-1559 USDC transfer, sign it on the card, broadcast it on Sepolia. |

## How to run an example

1. **Install the library** by running `setup.bat` from the repository
   root (Windows). The script clones every pinned third-party
   dependency, applies the required patches, and runs the
   memory-optimisation pass — without it the sketches may fail to
   compile or overflow flash. See [installation](../README.md#installation).
2. **Restart the Arduino IDE.**
3. **Open the sketch** via *File → Examples → CryptnoxWallet → \<example\>*.
4. **Select the board**: *Tools → Board → Arduino UNO R4 Minima* (or
   *WiFi* — required for `UsdcSigning`).
5. **Select the serial port**: *Tools → Port → ...*
6. **(UsdcSigning only)** copy `config.template.h` to `config.h` and
   fill in your Wi-Fi credentials, RPC endpoint, PIN, addresses and
   amount. `config.h` is gitignored — do not commit it.
7. **Compile and upload**, then open the **Serial Monitor** at
   *115200 baud*. Place the card on the PN532 antenna when prompted.

> [!NOTE]
> All examples default to PIN `000000000` (nine zeros). If your card
> was initialised with a different PIN, edit the `DEMO_PIN` /
> `DEFAULT_PIN` / `CARD_PIN` macro at the top of the sketch before
> uploading.

## Adding a new example

Follow the conventions used by the existing sketches:

- Place each sketch in its own subdirectory under `examples/`, named
  in **PascalCase** (the Arduino IDE requires the `.ino` file to share
  the directory name).
- Start every source file with the SPDX + copyright header used by
  the rest of the repository.
- Add Doxygen tags (`@file`, `@example`, `@brief`) to the `.ino` so
  the sketch surfaces correctly on the
  [generated docs site](https://cryptnox.github.io/cryptnox-sdk-arduino/).
- If the sketch needs external secrets (Wi-Fi credentials, API keys,
  recipient addresses), ship a `config.template.h` next to it and add
  the runtime `config.h` to `.gitignore`. Never commit credentials.
- Ship a `README.md` next to the sketch and register it in the
  [Available examples](#available-examples) table above.

## License

`cryptnox-sdk-arduino` is dual-licensed:

- **LGPL-3.0** for open-source projects and proprietary projects that comply with LGPL requirements
- **Commercial license** for projects that require a proprietary license without LGPL obligations (see [COMMERCIAL.md](../COMMERCIAL.md) for details)

For commercial inquiries, contact: contact@cryptnox.com
