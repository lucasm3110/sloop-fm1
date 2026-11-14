# Building and maintaining Lucas FM

Source branch: `codex/lucas-fm` in `lucasm3110/sloop-fm1`; upstream remains `isod89/sloop-fm1`. Baseline upstream commit: `bac9c05ea7627449405aa10e48da21d75b6c8c56`.

Use Linux x86-64, or Docker's Linux amd64 environment on macOS, following the root BUILDING.md. Python 3 + Pillow, the pinned JieLi pi32v2 toolchain, and the three SDK files from `AC79NN_SDK_V1.2.1_2023-12-13` are required. `tools/build.py` verifies the SDK hashes. The GitHub workflow `.github/workflows/firmware.yml` is the exact reproducible environment used for these builds; it downloads only those three SDK files and runs `sh build.sh`.

```sh
sh tools/get_toolchain.sh
# Install the pinned SDK as documented in BUILDING.md.
sh build.sh
sh tests/run_lucas.sh       # Node 22+, Python 3, host C compiler with ASan/UBSan
sh tests/run_tests.sh       # Complete retained upstream suite; see known baseline failures
```

The default package identity is `FM-1_803`, display identity `LUCAS FM 0.3`. Do not use SLOOP's `--release` identity override for a Lucas build. Outputs are the update-only `build/felucca.fwsc` and preloaded emulator `build/felucca.play.fwsc` with `.elf`, `.bin`, disassembly and attribution beside it. CI uploads the package before tests, with source SHA and package SHA-256; a completed build is not proof that every test passed.

The upstream baseline was built and run first, without firmware-source changes, in CI commit `dcbdafa` (workflow-only change). Linux CI is used because this Mac has no Docker/JieLi cross-compiler. Native host tests and actual firmware emulation run locally.

## Emulator

The native emulator source is `joakimeriksson/fm1-emu`, tested at `883e10f8b37d25dbac1786cc6370f459a8f3d4a4`. Build with `cargo build --release -p fm1-live -p fm1-emu`.

```sh
../fm1-emu/target/release/fm1-live --fwsc dist/lucas-fm-0.3.fwsc --midi none --mhz 240
# A collection can be loaded with --flash PATH_TO_MATCHING_EMULATOR_IMAGE.bin.
```

`--midi none` ensures the development runner does not listen to real MIDI devices. To reproduce the automated screenshots, recordings and diagnostic RAM measurements, place the tested `felucca.fwsc`, `felucca.play.fwsc`, `.elf`, `.bin` in a new directory and run:

```sh
python3 tools/lucas_v03_check.py build/tested --runner ../fm1-emu/target/release/fm1-live
python3 tools/lucas_emulator.py build/tested --runner ../fm1-emu/target/release/fm1-live --screens --stress
python3 tools/lucas_maximum_check.py build/tested --runner ../fm1-emu/target/release/fm1-live --voices 7
# --voices 8 probes the known overload boundary and is expected to fail the no-shedding assertion.
python3 tools/lucas_upload_check.py build/tested --runner ../fm1-emu/target/release/fm1-live
```

Screenshots are rendered by the actual firmware. Strict tests use `FM1_STRICT=1`, including timed LCD SPI, instruction costs, cache misses and alignment checks. The regular screenshot tests still use `FM1_LCD_TIMED=1`. **Do not restore a full flash image from an older package while testing a newer one:** the emulator restores its code sectors too. The harness generates fresh flash from the current package and overlays sample slots only. `FM1_BENCH_NOKEYS` suppresses scripted keys, so the harness removes it from scripted runs.

The hosted emulator at https://joakimeriksson.github.io/fm1-emu/ accepts **Open .fwsc…**. Load the provided custom file; selecting the stock SLOOP entry runs stock firmware. Hosted browser saves and native emulator flash are separate. The 0.3 play-ready package embeds the collection for both browser and native emulators. The personal package was opened and visibly verified in Chrome at 240 MHz. The browser editor's mock mode is only a companion-workflow test, not evidence of firmware execution.

## Modules and formats

- `lucas_state.h`: compact shared controls, defaults, and feature gate. Actual `felucca.c` enables `LUCAS_FM`; upstream host tests leave it disabled. Dedicated integration tests enable it.
- `lucas_ui.c`: named-control dispatch, shared instrument-parameter mapping and strip-rendered UI. The unreachable legacy decorative visualizer is excluded only for Lucas, preserving its upstream implementation. Six 40-pixel strips are hashed; only changed strips are sent to LCD. Legacy deep pages remain available.
- `lucas_loops.c`: validated ten-entry library, four dedicated decoder states, ramps and queued rhythm launches. It reuses the SLOOP ADPCM decode primitive without the rebased-pointer trick in the legacy sample engine.
- `lucas_fx.c`: shared EQ/drive/modulation/grain. Bypass skips ring-buffer processing, preserving CPU for instruments.
- `project.c`: extension marker `0x4C31` followed by 48 bytes packed in 24 unused drum synth parameter words. `LF_STORE` is the explicit mapping. No project-size or flash-address change; `project_t` is 3816 bytes within 3840.
- `tools/lucas_pack.py`: offline preparation, storage packing, validation and optional **emulator-only** flash generation.
- `web/editor.html`: collection importer in the existing Samples workflow; original backup/kit/song features retained.

A `.lucaspack` is JSON `format: lucas-fm-loops`, `version: 1`, with up to four base64 FSMP v1 slots plus per-slot byte count and CRC32. `preserve_slots` optionally names unused zero-based slots which transfer must keep. Every loop slot has the original 512-byte FSMP header, `rsv2[0] = 0x314C464C`, and 128 bytes of metadata before ADPCM data. Maximum four zones per loop slot. Each 24-byte metadata entry is `<12sBBBBHHI`: ASCII name, type (texture/tonal/rhythmic = 0/1/2), root (0–11 or 255 unknown), Follow flags (key=1, BPM=2), library ID (0–9), prepared BPM, bars and reserved. Original user sample and drum formats remain unchanged. Firmware reads the validated zones and metadata; upload validates the existing FSMP CRC. A collection fingerprint prevents a new collection from inheriting unrelated Follow overrides.

No active layer state resumes automatically. Effects, levels, Follow settings, instruments and note patterns persist. Sample bytes remain in their original four flash regions. A firmware-only package should not be confused with a complete song/sample backup.

## Licenses

Retain LICENSING.md, LICENSE and all upstream source/asset notices. New firmware/tools are GPL-3.0-only. Public field recordings are Thimras's CC0 Park Ambiences, with URLs and original hashes in `assets/loops-cc0/`. Rendered starter pads and the original CC0 drum arrangement are supplied separately from the engine source. Personal Desktop audio, derivative packs, source paths and kit files are excluded by `.gitignore`; they are not licensed or distributed as public assets.
