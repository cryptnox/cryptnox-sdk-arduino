<p align="center">
  <img src="https://github.com/user-attachments/assets/6ce54a27-8fb6-48e6-9d1f-da144f43425a"/>
</p>
<h3 align="center">cryptnox-sdk-arduino</h3>
<p align="center">Arduino library for managing Cryptnox Hardware Wallet</p>
<br/>
<br/>

[![Platform: Arduino R4](https://img.shields.io/badge/Platform-Arduino%20R4-blue.svg)](https://www.arduino.cc/)
[![License: LGPLv3](https://img.shields.io/badge/License-LGPLv3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)

---

# Development guide

This document describes how to set up a development environment for
`cryptnox-sdk-arduino`, how the project is organized, and how releases are
built for the Arduino Library Manager.

If you only want to **use** the library, follow the instructions in the
[README](README.md) — you do not need any of this.

---

## Project structure

```
cryptnox-sdk-arduino/
├── examples/                   # Arduino example sketches (BasicUsage, ...)
├── src/                        # Public library sources
│   ├── CryptnoxWallet.h        # Top-level umbrella header
│   └── cryptnox-sdk-cpp/       # ← Git submodule: cryptnox-sdk-cpp (C++ core)
├── library.properties          # Arduino Library Manager metadata
├── README.md
├── DEVELOPMENT.md              # (this file)
└── CONTRIBUTING.md
```

The C++ core SDK lives in its own repository,
[`cryptnox-sdk-cpp`](https://github.com/cryptnox/cryptnox-sdk-cpp), and is
consumed by this project as a **Git submodule** under `src/cryptnox-sdk-cpp`.

> [!IMPORTANT]
> Releases published to the Arduino Library Manager do **not** contain a
> submodule — the core SDK is **vendored** (copied in) at release time. See
> [Release process](#release-process) below.

---

## Prerequisites

- **[Arduino IDE](https://www.arduino.cc/en/software)** (version 2.x recommended)
- **[Git](https://git-scm.com/downloads)** with submodule support
- **Arduino UNO R4 board support** (*Tools* → *Board* → *Boards Manager* → `Arduino UNO R4 Boards`)
- A C++ toolchain for running local tests (optional):
  - **GCC** or **Clang** on Linux / macOS
  - **MSYS2** or **MinGW-w64** on Windows
- **[arduino-cli](https://arduino.github.io/arduino-cli/latest/)** (optional, for command-line builds and CI)

---

## Getting the source

Clone the repository **with its submodules** so the core SDK ends up
checked out at `src/cryptnox-sdk-cpp`:

```bash
git clone --recurse-submodules https://github.com/cryptnox/cryptnox-sdk-arduino.git
cd cryptnox-sdk-arduino
```

> [!TIP]
> If you forgot `--recurse-submodules`, run this from inside the repo:
> ```bash
> git submodule update --init --recursive
> ```

### Installing into the Arduino IDE (Windows — `setup.bat`)

On Windows, the repo ships a one-shot installer that:

1. backs up the current Arduino `libraries` folder (sibling directory
   suffixed with a timestamp),
2. copies the SDK into `libraries\CryptnoxWallet` (only the files the IDE
   needs — no patches, docs, or scripts),
3. clones each pinned third-party dependency at the version listed in
   `setup.bat` (AESLib, Adafruit_BusIO, Adafruit_PN532,
   ArduinoHttpClient, Crypto, micro-ecc, trng),
4. applies the **required** patches from `patches\` (AESLib Renesas
   compile fix + iostream strip, PN532 timeout, PN532 extended-frame
   APDU support) — sketches will not compile or will silently truncate
   APDUs longer than 255 bytes without them,
5. runs `scripts\enable_memory_optimization.bat` to drop the AVR-compat
   float-printf hard pull and add `-fno-exceptions -fno-rtti` + uECC
   curve trimming to the Renesas core (≈ 156 KB flash saved end-to-end).

#### Prerequisites

- **Arduino IDE 2.x** with the **Arduino UNO R4 Boards** package
  installed via Boards Manager (the script patches the core that ships
  in that package).
- **git** in `PATH` (https://git-scm.com/) — the script uses git as the
  install transport for the third-party libraries.

#### Run

From the repository root in `cmd.exe` or PowerShell:

```bat
setup.bat
```

To re-run from a clean slate (wipes the whole `Arduino\libraries`
directory after backing it up):

```bat
setup.bat --reset
```

To target a non-default libraries directory:

```bat
setup.bat "D:\my\custom\Arduino\libraries"
```

When the script finishes, **restart the Arduino IDE** so it picks up the
new libraries and the modified core.

> [!IMPORTANT]
> macOS / Linux: no installer is shipped yet. Clone the repo into your
> `Arduino/libraries` folder, install the third-party libraries listed
> above, and then apply each patch under `patches\` by hand with
> `patch -p1` — none of them is optional (the PN532 extended-frame
> patch is required for certificate pages > 255 bytes to deliver
> intact, and the AESLib Renesas patch is required for the library to
> even compile on the UNO R4).

The final path should be `.../Arduino/libraries/CryptnoxWallet`, with the
core SDK at `.../CryptnoxWallet/src/cryptnox-sdk-cpp`.

### Verify the submodule

The `src/cryptnox-sdk-cpp` directory must **not** be empty:

```bash
ls src/cryptnox-sdk-cpp
```

If it is empty, re-run `git submodule update --init --recursive`.

### Updating the submodule

To pull the latest upstream changes of `cryptnox-sdk-cpp` into your working
copy:

```bash
git submodule update --remote --merge src/cryptnox-sdk-cpp
git add src/cryptnox-sdk-cpp
git commit -m "chore(sdk-cpp): bump to <commit-or-tag>"
```

Pin the submodule to a specific tag or commit when preparing a release — do
**not** track a moving branch for published versions.

---

## Building & testing

### Building the example from the IDE

1. Open the Arduino IDE.
2. It should auto-detect the library in your `libraries` folder.
3. Open *File* → *Examples* → **cryptnox-sdk-arduino** → **BasicUsage**.
4. Select your board and port, then compile.

### Building from the command line (arduino-cli)

```bash
arduino-cli core install arduino:renesas_uno
arduino-cli compile \
  --fqbn arduino:renesas_uno:unor4wifi \
  examples/BasicUsage/BasicUsage.ino
```

Replace `unor4wifi` with `minima` for the UNO R4 Minima.

### Hardware-in-the-loop testing

Most of the library can only be meaningfully tested against a real
Cryptnox Hardware Wallet. Before running the signing path, the card must be
provisioned with a seed.

**Quickest — using the [Cryptnox CLI](https://github.com/cryptnox/cryptnox-cli):**

```bash
cryptnox init
cryptnox seed generate
```

**Scripted — using [cryptnox-sdk-py](https://github.com/cryptnox/cryptnox-sdk-py):**

```python
card.init(pin)
card.generate_seed(pin)     # or: card.load_seed(seed, pin)
```

Use the default PIN `000000000` unless you changed it during initialization.

---

## Coding style

- Follow the existing style: 4-space indentation, K&R braces, `snake_case`
  for local variables, `camelCase` for methods, and `UPPER_SNAKE_CASE` for
  macros and constants.
- Public API headers live in `src/` and must be self-contained (no leaking
  submodule paths).
- Prefix all public types and macros with `CW_` (CryptnoxWallet) or
  `Cryptnox` to avoid clashes with other Arduino libraries.
- Wrap logging through `ArduinoLoggerAdapter` — never call `Serial` directly
  in library code.
- Wipe any sensitive buffer (PIN, seed, private key, signature) with
  `CW_Utils::secure_wipe` before it goes out of scope.

### Commit messages

Use [Conventional Commits](https://www.conventionalcommits.org/):

```
feat(wallet): add verifyPin() helper
fix(pn532): handle timeout during anticollision
chore(sdk-cpp): bump to v1.6.1
docs(readme): clarify seed provisioning step
```

---

## Release process

Releases are published via the **Arduino Library Manager**, which pulls
tagged archives from GitHub. The Library Manager does **not** resolve Git
submodules, so the `cryptnox-sdk-cpp` code must be **vendored** (copied in)
before tagging.

### 1. Prepare the release branch

```bash
git checkout -b release/vX.Y.Z
git submodule update --remote --merge src/cryptnox-sdk-cpp   # pin latest core SDK
```

### 2. Bump the version

Update `library.properties`:

```properties
name=CryptnoxWallet
version=X.Y.Z
author=Cryptnox SA
...
```

### 3. Vendor the submodule

> [!WARNING]
> The commands below use `-f` and `rm -rf` and will **not** warn you about
> uncommitted changes. Make sure your working tree is clean first:
> ```bash
> git status                       # must show "nothing to commit"
> git -C src/cryptnox-sdk-cpp status   # same for the submodule
> ```
> Commit or stash any pending work before proceeding.

Replace the submodule with a plain copy of its current checkout:

```bash
# Record the upstream commit for traceability
SDK_CPP_COMMIT=$(git -C src/cryptnox-sdk-cpp rev-parse HEAD)
echo "cryptnox-sdk-cpp @ ${SDK_CPP_COMMIT}" > src/cryptnox-sdk-cpp/.VENDORED_FROM

# De-submodule: keep the files, drop the Git metadata
git submodule deinit -f src/cryptnox-sdk-cpp
git rm -f src/cryptnox-sdk-cpp
rm -rf .git/modules/src/cryptnox-sdk-cpp
git clone --depth=1 https://github.com/cryptnox/cryptnox-sdk-cpp.git src/cryptnox-sdk-cpp
rm -rf src/cryptnox-sdk-cpp/.git src/cryptnox-sdk-cpp/.github
git add src/cryptnox-sdk-cpp
```

At this point the release branch contains a full, self-contained copy of
the core SDK. The `main`/`master` branch **keeps the submodule** — never
merge the vendored branch back.

### 4. Tag and push

```bash
git commit -m "release: vX.Y.Z"
git tag -a vX.Y.Z -m "cryptnox-sdk-arduino vX.Y.Z"
git push origin release/vX.Y.Z --tags
```

### 5. Publish to the Library Manager

- Create a GitHub Release from the tag. Attach auto-generated release notes.
- If this is the first release, submit the repository to the
  [Arduino Library Manager registry](https://github.com/arduino/library-registry).
  For subsequent releases, the registry picks up new tags automatically
  within ~1 hour.

### 6. Verify

- Open the Arduino IDE, refresh the Library Manager, and confirm
  `cryptnox-sdk-arduino` shows the new version.
- Install it in a clean Arduino installation and compile `BasicUsage` to
  make sure the vendored code works end-to-end.

---

## Getting help

- **Bugs / feature requests:** open an issue on
  [github.com/cryptnox/cryptnox-sdk-arduino/issues](https://github.com/cryptnox/cryptnox-sdk-arduino/issues).
- **Commercial / licensing:** contact@cryptnox.com
