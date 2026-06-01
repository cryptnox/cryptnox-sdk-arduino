<div align="center">

<img src="https://github.com/user-attachments/assets/6ce54a27-8fb6-48e6-9d1f-da144f43425a"/>

### cryptnox-sdk-arduino

Arduino library for managing Cryptnox Hardware Wallet

</div>

# UsdcSigning — Broadcast a Real EIP-1559 USDC Transfer on Sepolia

End-to-end demonstration of the Cryptnox Hardware Wallet on an Arduino
UNO R4 WiFi: connect to a Wi-Fi access point, fetch the nonce and fee
parameters over JSON-RPC, build and Keccak-256-hash the unsigned
EIP-1559 transaction, sign it on the card
(BIP-44 `m/44'/60'/0'/0/0`, secp256k1, canonical low-S), recover
`yParity`, broadcast the signed transaction, and print the on-chain
tx hash.

The private key never leaves the card. The Arduino only ever holds
the `Account` derived from the card's public key, the unsigned tx
hash, and the resulting `(r, s)`.

> [!WARNING]
> Every wrong PIN attempt decrements an on-card retry counter. At
> zero the PIN is permanently blocked. The sketch halts on
> `CW_SIGN_PIN_INCORRECT` — verify `CARD_PIN` in `config.h` matches
> the value used during `cryptnox init` **before** uploading.

## Requirements

| Component | Details |
|-----------|---------|
| **Hardware Wallet** | Cryptnox Hardware Wallet, initialised **and** seeded |
| **Wallet funding** | The address derived from `m/44'/60'/0'/0/0` on the card needs Sepolia ETH (for gas) and Sepolia USDC. Faucets: [Sepolia ETH](https://sepoliafaucet.com/) · [Circle USDC](https://faucet.circle.com/) |
| **NFC reader** | PN532 over SPI — see [hardware setup](../../README.md#hardware-setup) |
| **Board** | **Arduino UNO R4 WiFi** (the Minima has no Wi-Fi) |
| **RPC endpoint** | A Sepolia JSON-RPC endpoint — [PublicNode](https://ethereum-sepolia-rpc.publicnode.com) (no signup, default) or [Infura](https://app.infura.io/) |
| **SDK** | Installed via `setup.bat` — see [installation](../../README.md#installation) |

## Quick start

1. **Create `config.h`** by copying the template:

   ```bash
   cp config.template.h config.h
   ```

   Fill in at minimum: `WIFI_SSID`, `WIFI_PASSWORD`, `CARD_PIN`,
   `ADDR_FROM` (the address derived from `m/44'/60'/0'/0/0` on the
   card), `ADDR_TO` (recipient), `AMOUNT_USDC` (token base units —
   1 USDC = 1 000 000).

   > [!IMPORTANT]
   > `config.h` is gitignored — never commit it.

2. *File → Examples → CryptnoxWallet → UsdcSigning*

3. *Tools → Board → **Arduino UNO R4 WiFi*** (not Minima) and
   *Tools → Port*

4. **Upload**, open the **Serial Monitor** at *115200 baud*, place the
   card on the PN532 antenna when prompted.

### Expected output

```
PN532 OK
Connecting to WiFi....
fetchNonce: connecting to ethereum-sepolia-rpc.publicnode.com/
fetchNonce: status=200 nonce=0x5
[HASH]: 0x6679a2cd3064046397addbb97004b606df9281f624409fd36d2d24832db59c29
Place Cryptnox card on PN532 reader...
Card connected, secure channel established.
[SIG r]: 7C1F3A925E0B8CD4920FAB7C53E1B9D81F2A4C76E9D805BC5D2EAD12C3FA47CB
[SIG s]: 12E0BC4FA7880967DA1F0EBD83C2B791580AC4D62E16039F7B4CDDF13E2A89C5
yParity: 1
Sending...
[RPC] HTTP 200
[tx] hash=0xab12cd34ef56...
Transaction sent successfully!
```

Paste the final `[tx] hash` into
[Sepolia Etherscan](https://sepolia.etherscan.io/) to watch
confirmation.

## How it works

```
 1. WiFi.begin(WIFI_SSID, WIFI_PASSWORD)
        │
 2. fetchNonce()                      eth_getTransactionCount over
        │                             TLS-pinned WiFiSSLClient
        │
 3. Build Tx2 struct                  chainId=11155111, nonce, fees,
        │                             gasLimit, to, value=0, data
        │
 4. encodeERC20Transfer()             0xa9059cbb || pad(to,32) ||
        │                             pad(AMOUNT_USDC,32)
        │
 5. rlpEncodeUnsignedTx()             0x02 || rlp([chainId, nonce,
        │                             ..., [] accessList])
        │
 6. keccak256(rlp, len, hash)         EIP-2718 typed-tx pre-image (32 B)
        │
 7. wallet.connect(session)           Secure channel
        │
 8. Build CW_SignRequest:
      keyType   = CW_SIGN_DERIVE_K1   (BIP-44 m/44'/60'/0'/0/0)
      sigType   = CW_SIGN_SIG_ECDSA_LOW_S
      pinMode   = CW_SIGN_WITH_PIN
      hash      = keccak256(unsigned tx)
      pin       = CARD_PIN
        │
 9. wallet.sign(req)                  64-byte r || s
        │
10. determineYParity(hash, r, s)      Call the ecrecover precompile
        │                             with v=27, then v=28 — keep the
        │                             one that returns ADDR_FROM
        │
11. rlpEncodeSignedTx()               Same RLP plus the signature fields
        │
12. sendRawTx()                       eth_sendRawTransaction → extract
        │                             "result":"0x…" tx hash
        │
13. secure_wipe(req.pin, ...)         Zero the PIN buffer
```

### TLS pinning

`WiFiSSLClient` is initialised with `setCACert(WIFI_CA_CERT)`. The
sketch ships with **ISRG Root X1** (Let's Encrypt — *Internet
Security Research Group*) as a placeholder default. That root does
**not** match PublicNode's current chain
(`ethereum-sepolia-rpc.publicnode.com` is issued by Google Trust
Services R4) nor Infura's (DigiCert), so for any real endpoint you
will need to override `WIFI_CA_CERT` in `config.h`. Retrieve the
correct root with:

```bash
openssl s_client -showcerts -servername HOST -connect HOST:443 </dev/null
```

For development only you can set `WIFI_DISABLE_CA_PINNING` in
`config.h` to skip validation. The sketch prints a loud warning at
boot when this is set.

> [!CAUTION]
> Never ship firmware with `WIFI_DISABLE_CA_PINNING` defined — the
> connection becomes trivially MITM-able and an attacker can feed
> the device crafted nonces / fees and have it sign whatever they
> want.

### `yParity` recovery

EIP-1559 signatures carry a 1-bit `yParity` instead of EIP-155's
`v`. The card returns only `r` and `s`, so the sketch calls the
[`ecrecover` precompile](https://www.evm.codes/precompiled?fork=cancun)
at address `0x01` with `v = 27` (yParity = 0). If the recovered
address matches `ADDR_FROM` the sketch keeps yParity = 0; otherwise
it retries with `v = 28` (yParity = 1). One of the two will match
when the card's key and `ADDR_FROM` are consistent.

## Configuration reference

All fields live in `config.h` (gitignored). Start from
[`config.template.h`](config.template.h) and fill in:

### Wi-Fi

| Field | Required | Example |
|-------|:--------:|---------|
| `WIFI_SSID` | yes | `"MyHomeNetwork"` |
| `WIFI_PASSWORD` | yes | `"password"` |

> [!NOTE]
> The Arduino UNO R4 WiFi only supports **2.4 GHz** Wi-Fi.

### RPC endpoint — choose one provider

**PublicNode** (free, no signup, default):

```c
#define RPC_HOST  "ethereum-sepolia-rpc.publicnode.com"
#define RPC_PORT  443
#define RPC_PATH  "/"
```

**Infura** (free tier, requires API key):

```c
#define RPC_HOST        "sepolia.infura.io"
#define RPC_PORT        443
#define RPC_PROJECT_ID  "<YOUR_INFURA_PROJECT_ID>"
#define RPC_PATH        "/v3/" RPC_PROJECT_ID
#define RPC_API_SECRET  "<YOUR_INFURA_API_SECRET>"   // HTTP Basic Auth
```

### TLS

| Field | Default | Notes |
|-------|---------|-------|
| `WIFI_CA_CERT` | ISRG Root X1 PEM (placeholder bundled in the sketch) | **Almost always needs overriding** — set it to the root CA your RPC provider uses |
| `WIFI_DISABLE_CA_PINNING` | unset | Defining it disables TLS validation — DEV ONLY |

### Wallet

| Field | Example |
|-------|---------|
| `CARD_PIN` | `"000000000"` (must match `cryptnox init`) |
| `CARD_PIN_LEN` | `9U` (ASCII digit count) |
| `ADDR_FROM` | `"4aadf6f331aea39e699db17c75a4b12d993956d2"` (lowercase hex, no `0x`) |

`ADDR_FROM` must equal the address derived from `m/44'/60'/0'/0/0`
on the card. If they disagree, the `yParity` recovery loop cannot
find a matching value and the sketch halts.

### Transaction

| Field | Default | Notes |
|-------|---------|-------|
| `ADDR_TO` | `"Cd7E5...c06e"` | Recipient address (hex, no `0x`) |
| `ADDR_USDC` | `"1c7D4...7238"` | USDC contract on Sepolia |
| `CHAIN_ID_SEPOLIA` | `11155111` | Hardcoded — no path to broadcast on mainnet by accident |
| `AMOUNT_USDC` | `1000000UL` | 1 USDC (6 decimals) |
| `MAX_PRIORITY_FEE` | `2000000000ULL` | 2 Gwei |
| `MAX_FEE` | `4000000000ULL` | 4 Gwei |
| `GAS_LIMIT_ERC20` | `60000ULL` | Standard ERC-20 transfer |

## Memory footprint

After `setup.bat`'s memory-optimisation pass:

| Section | Size |
|---------|-----:|
| Flash (`.text` + `.data`) | ~99 KB / 256 KB (38 %) |
| RAM (`.bss` + `.data`) | ~11.6 KB / 32 KB (35 %) |

The bulk of the flash (~25 KB) is the WiFi/HTTP stack plus the
bundled CA certificate (~1.4 KB PEM). The Cryptnox SDK itself,
together with the RLP / Keccak-256 helpers, sits at around ~20 KB.

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `fetchNonce: POST failed, err=-1` | TLS handshake fails (wrong CA pinned) | Re-extract the chain with `openssl s_client -showcerts -servername HOST -connect HOST:443 </dev/null` and put the root in `WIFI_CA_CERT`. For a quick diagnosis, temporarily set `WIFI_DISABLE_CA_PINNING` to confirm the cert is the blocker |
| `Connecting to WiFi....` (never stops) | Wrong SSID / password, or a 5 GHz-only network | UNO R4 WiFi is 2.4 GHz only — verify SSID, password, band |
| `[fatal] tx.to is not a valid 40-char hex string` | Bad hex in `ADDR_TO` | 40 hex chars, no `0x`, lowercase preferred |
| `yParity determination failed!` | `ADDR_FROM` doesn't match the card's m/44'/60'/0'/0/0 address | Derive the address with the [Cryptnox CLI](https://github.com/cryptnox/cryptnox-cli) and copy it into `ADDR_FROM` |
| Tx broadcast OK but `tefPAST_NONCE` on Etherscan | Stale nonce from the RPC | Wait a few seconds and re-run; the sketch fetches the nonce just before signing |
| `Wrong PIN — halting to protect retry counter` | `CARD_PIN` does not match the card | Fix `CARD_PIN` — do **not** keep retrying, every attempt burns one of the on-card tries (see [VerifyPin](../VerifyPin/README.md)) |
| `Sign failed: 0x81` | Card has no seed | `cryptnox seed generate` |

## License

`cryptnox-sdk-arduino` is dual-licensed:

- **LGPL-3.0** for open-source projects and proprietary projects that comply with LGPL requirements
- **Commercial license** for projects that require a proprietary license without LGPL obligations (see [COMMERCIAL.md](../../COMMERCIAL.md) for details)

For commercial inquiries, contact: contact@cryptnox.com
