/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (c) 2026 Cryptnox SA
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* =========================
 * WiFi Configuration
 * ========================= */
#define WIFI_SSID        "<YOUR_SSID>"
#define WIFI_PASSWORD    "<YOUR_PASSWORD>"

/* =========================
 * Ethereum / RPC
 * ========================= */
/**
 * Choose ONE provider below and comment out the other.
 *
 * Option A — PublicNode (free, no account required)
 *   Uncomment the three lines under "Option A".
 *
 * Option B — Infura (requires a free account at app.infura.io)
 *   1. Create an API key in the Infura dashboard.
 *   2. In the key's Settings tab, reveal (or generate) the API Secret.
 *   3. Uncomment the five lines under "Option B" and fill in the values.
 *   Note: the API Secret must have NO leading or trailing spaces.
 */

/* --- Option A: PublicNode ----------------------------------------- */
#define RPC_HOST       "ethereum-sepolia-rpc.publicnode.com"
#define RPC_PORT       443
#define RPC_PATH       "/"
/* No authentication needed — leave RPC_PROJECT_ID / RPC_API_SECRET
 * undefined (or comment them out) when using PublicNode.            */

/* --- Option B: Infura --------------------------------------------- */
/* #define RPC_HOST        "sepolia.infura.io"                        */
/* #define RPC_PORT        443                                         */
/* #define RPC_PROJECT_ID  "<YOUR_INFURA_PROJECT_ID>"                 */
/* #define RPC_PATH        "/v3/" RPC_PROJECT_ID                      */
/* #define RPC_API_SECRET  "<YOUR_INFURA_API_SECRET>"                 */

/* --- TLS server certificate pinning (M-04) -----------------------
 * The sketch defaults to ISRG Root X1 (Let's Encrypt). If your provider
 * uses a different root CA, override WIFI_CA_CERT below with the
 * appropriate root in PEM form.
 *
 * To retrieve the real CA chain of your endpoint:
 *   openssl s_client -showcerts -servername HOST -connect HOST:443 </dev/null
 *
 * To DISABLE pinning temporarily for development (e.g. to confirm a TLS
 * handshake failure is a cert issue, not WiFi), uncomment WIFI_DISABLE_-
 * CA_PINNING. The sketch prints a loud warning at boot when set.
 * ⚠️ Never ship firmware with WIFI_DISABLE_CA_PINNING defined — the
 * connection becomes trivially MITM-able. */
#define WIFI_CA_CERT \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDejCCAmKgAwIBAgIQf+UwvzMTQ77dghYQST2KGzANBgkqhkiG9w0BAQsFADBX\n" \
"MQswCQYDVQQGEwJCRTEZMBcGA1UEChMQR2xvYmFsU2lnbiBudi1zYTEQMA4GA1UE\n" \
"CxMHUm9vdCBDQTEbMBkGA1UEAxMSR2xvYmFsU2lnbiBSb290IENBMB4XDTIzMTEx\n" \
"NTAzNDMyMVoXDTI4MDEyODAwMDA0MlowRzELMAkGA1UEBhMCVVMxIjAgBgNVBAoT\n" \
"GUdvb2dsZSBUcnVzdCBTZXJ2aWNlcyBMTEMxFDASBgNVBAMTC0dUUyBSb290IFI0\n" \
"MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAE83Rzp2iLYK5DuDXFgTB7S0md+8Fhzube\n" \
"Rr1r1WEYNa5A3XP3iZEwWus87oV8okB2O6nGuEfYKueSkWpz6bFyOZ8pn6KY019e\n" \
"WIZlD6GEZQbR3IvJx3PIjGov5cSr0R2Ko4H/MIH8MA4GA1UdDwEB/wQEAwIBhjAd\n" \
"BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDwYDVR0TAQH/BAUwAwEB/zAd\n" \
"BgNVHQ4EFgQUgEzW63T/STaj1dj8tT7FavCUHYwwHwYDVR0jBBgwFoAUYHtmGkUN\n" \
"l8qJUC99BM00qP/8/UswNgYIKwYBBQUHAQEEKjAoMCYGCCsGAQUFBzAChhpodHRw\n" \
"Oi8vaS5wa2kuZ29vZy9nc3IxLmNydDAtBgNVHR8EJjAkMCKgIKAehhxodHRwOi8v\n" \
"Yy5wa2kuZ29vZy9yL2dzcjEuY3JsMBMGA1UdIAQMMAowCAYGZ4EMAQIBMA0GCSqG\n" \
"SIb3DQEBCwUAA4IBAQAYQrsPBtYDh5bjP2OBDwmkoWhIDDkic574y04tfzHpn+cJ\n" \
"odI2D4SseesQ6bDrarZ7C30ddLibZatoKiws3UL9xnELz4ct92vID24FfVbiI1hY\n" \
"+SW6FoVHkNeWIP0GCbaM4C6uVdF5dTUsMVs/ZbzNnIdCp5Gxmx5ejvEau8otR/Cs\n" \
"kGN+hr/W5GvT1tMBjgWKZ1i4//emhA1JG1BbPzoLJQvyEotc03lXjTaCzv8mEbep\n" \
"8RqZ7a2CPsgRbuvTPBwcOMBBmuFeU88+FSBX6+7iP0il8b4Z0QFqIwwMHfs/L6K1\n" \
"vepuoxtGzi4CZ68zJpiq1UvSqTbFJjtbD4seiMHl\n" \
"-----END CERTIFICATE-----\n"
/* #define WIFI_DISABLE_CA_PINNING */  /* DEV ONLY — MITM vulnerable */

/* =========================
 * Wallet / Keys (SENSITIVE)
 * ========================= */
/* ⚠️ NEVER COMMIT config.h — it contains credentials.
 *
 * L-04 — A hardcoded PIN sits in flash (.rodata) and is recoverable via
 * SWD/JTAG. OK for a demo, NOT for production: in prod replace with a
 * keypad / BLE prompt / companion chip, then secure_wipe() the buffer
 * right after wallet.sign() / wallet.verifyPin(). */

#define CARD_PIN      "<CARD_PIN>"   /* 4-9 digit PIN, e.g. "000000000" */
#define CARD_PIN_LEN  (9U)           /* number of digits in CARD_PIN     */

/* =========================
 * Ethereum Addresses
 * ========================= */
/* Sender address — lowercase hex, no 0x prefix */
#define ADDR_FROM         "<SENDER_ADDRESS>"

/* Recipient address */
#define ADDR_TO           "<RECIPIENT_ADDRESS>"

/* USDC contract address (Sepolia testnet) */
#define ADDR_USDC         "<USDC_CONTRACT_ADDRESS>"

/* =========================
 * Transaction Parameters
 * ========================= */
#define CHAIN_ID_SEPOLIA  11155111

/* Amount in token smallest unit (USDC has 6 decimals) */
#define AMOUNT_USDC       1000000UL   /* 1.0 USDC */

/* Gas parameters (in wei) */
#define MAX_PRIORITY_FEE  2000000000ULL  /* 2 Gwei */
#define MAX_FEE           4000000000ULL  /* 4 Gwei */
#define GAS_LIMIT_ERC20   60000ULL

#endif /* CONFIG_H */
