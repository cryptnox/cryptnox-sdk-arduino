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

# Contributing to cryptnox-sdk-arduino

Thanks for your interest in contributing! This document explains how to
propose changes, what we expect from a pull request, and how to get your
work reviewed and merged.

For the development setup (cloning with submodules, building, testing),
see [DEVELOPMENT.md](DEVELOPMENT.md).

---

## Ways to contribute

- **Report a bug** — open an [issue](https://github.com/cryptnox/cryptnox-sdk-arduino/issues)
  with a clear title, the Arduino board and firmware version you're using,
  and a minimal sketch that reproduces the problem.
- **Suggest a feature** — open an issue describing the use case. Please
  check the existing issues first to avoid duplicates.
- **Improve documentation** — fixes to the README, code comments, or the
  generated API docs are always welcome.
- **Submit code** — bug fixes, new examples, or new features. See
  [Pull request workflow](#pull-request-workflow) below.

---

## Before you start

1. Check existing [issues](https://github.com/cryptnox/cryptnox-sdk-arduino/issues)
   and [pull requests](https://github.com/cryptnox/cryptnox-sdk-arduino/pulls)
   to make sure the change is not already in progress.
2. For non-trivial changes, open an issue first to discuss the approach.
   This avoids wasted work if the maintainers have a different direction in
   mind.
3. Keep changes focused. One PR = one logical change. Split refactors,
   features, and unrelated fixes into separate PRs.

---

## Pull request workflow

### 1. Fork & clone

Fork the repository on GitHub, then clone your fork **with submodules**:

```bash
git clone --recurse-submodules https://github.com/<your-username>/cryptnox-sdk-arduino.git
cd cryptnox-sdk-arduino
git remote add upstream https://github.com/cryptnox/cryptnox-sdk-arduino.git
```

### 2. Create a branch

Branch from `main` using a descriptive name:

```bash
git checkout -b feat/verify-pin-helper
# or: fix/pn532-i2c-timeout, docs/readme-seed-provisioning, etc.
```

### 3. Make your changes

- Follow the [coding style](DEVELOPMENT.md#coding-style).
- Keep commits small and focused.
- Write [Conventional Commit](https://www.conventionalcommits.org/)
  messages:
  ```
  feat(wallet): add verifyPin() helper
  fix(pn532): handle timeout during anticollision
  docs(readme): clarify seed provisioning step
  ```
- Update documentation (README, code comments, examples) when user-facing
  behavior changes.
- Add or update an example sketch if you introduce a new public API.

### 4. Test locally

At minimum:

- The `BasicUsage` example must still compile for both `USE_SPI` and
  `USE_I2C` configurations.
- If you have the hardware, run the example end-to-end against a real
  Cryptnox card.

See [DEVELOPMENT.md](DEVELOPMENT.md#building--testing) for build commands.

### 5. Push and open a pull request

```bash
git push origin feat/verify-pin-helper
```

Then open a pull request against `main`. In the description, include:

- **What** the change does.
- **Why** it is needed (link the issue if applicable, e.g. `Fixes #42`).
- **How** you tested it (IDE version, board, card firmware, interface).
- Any breaking changes or migration notes.

### 6. Review

A maintainer will review your PR within a few working days. Expect
feedback — most PRs need at least one round of changes. Please:

- Respond to comments rather than closing and reopening.
- Push fixup commits to the same branch; the maintainer will squash on
  merge unless the history is meaningful.
- Keep the conversation technical and respectful.

---

## What makes a good pull request

- **Focused.** One concern per PR.
- **Small.** Aim for < 300 lines of diff when possible. Larger changes
  should be split or discussed in an issue first.
- **Tested.** At least a compile pass with `arduino-cli`; ideally a run
  against real hardware.
- **Documented.** Public API changes come with doc comments and, when
  relevant, an updated example sketch.
- **No unrelated reformatting.** If you run a formatter, do it in a
  separate PR.
- **Submodule untouched** unless the PR is specifically about bumping the
  core SDK. If you do bump it, mention it in the PR title
  (`chore(sdk-cpp): bump to vX.Y.Z`).

---

## Reporting security issues

**Do not open a public issue for security vulnerabilities.** Email us
privately at [contact@cryptnox.com](mailto:contact@cryptnox.com) with:

- A description of the vulnerability.
- Steps to reproduce.
- The affected version(s).
- Your contact information for follow-up.

We will acknowledge receipt within 5 working days and coordinate a fix and
disclosure timeline with you.

---

## Licensing

By submitting a pull request, you agree that your contribution will be
licensed under the same terms as the project (LGPL-3.0, with the option of
the commercial license described in [COMMERCIAL.md](COMMERCIAL.md)).

If your employer holds the copyright on your contribution, make sure they
are aware of and agree to this before submitting.

---

## Code of conduct

Be respectful. We welcome contributors of all backgrounds and experience
levels. Harassment, personal attacks, and discriminatory language will not
be tolerated in issues, pull requests, or any other project space.
