# Building Lucas Loops

Branch: `codex/lucas-fm` in `lucasm3110/sloop-fm1`. Upstream SLOOP baseline: `bac9c05ea7627449405aa10e48da21d75b6c8c56`. This project is separate from TeleNeuMu.

Firmware uses the pinned JieLi pi32v2 Linux toolchain and SDK described in root BUILDING.md. `.github/workflows/firmware.yml` reproduces the build. Run `sh build.sh`, then `sh tests/run_lucas.sh` and `sh tests/run_tests.sh`. Local Lucas tests require Node 22+, Python/Pillow, ffmpeg and a host C compiler with sanitizers. Package identity is FM-1_804 / LUCAS LOOPS 0.4.

## Browser

`Open Lucas Loops.command` expands the bundled public runtime into ignored `build/loops-browser/`, overlays the personal package if present, serves loopback port 8774 and opens the browser. It installs nothing and accesses no MIDI hardware. Source for the panel is `web/loops/`; it sends button/encoder events to the real emulator and displays its LCD frames.

To rebuild the runtime, use sibling `joakimeriksson/fm1-emu` at commit `883e10f8b37d25dbac1786cc6370f459a8f3d4a4`, install Rust's wasm32-unknown-unknown target, and run `bash ../fm1-emu/web/build.sh build/browser-engine`. Copy its fm1_web.wasm, worklet.js and persistence.js into `engine/`, copy web/loops files, assets/fonts/Nunito.ttf, and the desired play-ready package as firmware.fwsc. Serve via localhost (AudioWorklet requires a secure context). Never put personal packages in public archives.

## Native verification

Build the same emulator with `cargo build --release -p fm1-live`. The native launcher uses --midi none and separate 0.4 session files. For reproducible strict tests, put final felucca.elf/.bin/.fwsc and felucca.play.fwsc in a fresh directory, then:

```sh
python3 tools/lucas_v04_check.py build/tested --runner ../fm1-emu/target/release/fm1-live
```

Do not restore full flash from older firmware: it includes code sectors. The 0.4 test creates a fresh flash file for its SAVE/reboot pair.

## Source

- lucas_ui.c: only home and sound chooser; strip rendering with change hashes.
- lucas_scene.c: four ADPCM decoders, shared bar clock, fade gains and queued replacement.
- lucas_state.h / project.c: 48-byte state extension, marker 0x4C32; previous 0x4C31 state migrates to defaults. SAVE writes the working autosave object, which restores on boot.
- tools/lucas_collection.py and tests/lucas_loop_render.c: reproducible tonal rendering and public/personal loop packs. `--kit private/kit` builds personal rhythm arrangements.
- tools/lucas_preload.py: insert loop collection into emulator-ready firmware package.

Retain LICENSE, LICENSING.md, Nunito SIL OFL, fm1-emu MIT and sample credits. Public source is GPL-3.0-only. Personal Desktop audio and derived packages stay in ignored private/ or build/ folders.
