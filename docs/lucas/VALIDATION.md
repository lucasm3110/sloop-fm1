# Lucas FM 0.1 — verification and limits

Historical release report. See [0.2 changes and validation](0.2.md) for the current package.

This is an actual compiled FM-1 firmware fork, tested with the native FM-1 emulator. **No physical FM-1 was flashed. Physical audio, USB, controls, recovery and long-session reliability remain unverified.** TeleNeuMu and Geara are unrelated and were not modified.

## Reproducible identity

The exact compiled source commit is in `dist/source-commit-0.1.txt`; `dist/SHA256SUMS` identifies the delivered package. The delivery/documentation commit can be newer than that source commit without changing the firmware. Upstream: `bac9c05ea7627449405aa10e48da21d75b6c8c56`. Native emulator: `883e10f8b37d25dbac1786cc6370f459a8f3d4a4`. See DEVELOPMENT.md for build and replay commands.

Baseline SLOOP was compiled and run before firmware changes. Every automated release replay starts with flash generated from its own package: restoring old full emulator flash would also restore old code. The package identity is FM-1_801, with LUCAS FM 0.1 on screen.

## Feature acceptance

| Requirement | Implemented and checked | Qualification |
|---|---|---|
| Six main destinations | Actual compiled LCD screenshots; physical button/encoder dispatch | HOME provides universal return; retained deep SLOOP pages have their original UI |
| Legible 240 × 240 UI | Track colors, chord wheel, curved preset list, timelines, knob labels | Reference-inspired layout, not screenshot reproduction; single-color physical key LEDs |
| Number mode | All twelve roots, seven diatonic qualities, root display, note/chord choice | White-key mapping; black keys silent in everyday Number mode |
| Guided piano | Chromatic mapping and scale guides | Out-of-scale notes remain playable |
| Hold and release | Host assertions for hold, mode/key/track changes; actual held-chord replay | Eight pitched voices shared by three tracks; original voice allocation remains |
| Sequencing | Retained recording, overdub, undo, mute, sections; actual record/save/reboot/play replay | Deep editing remains SLOOP's interface; no audio recording into sequence lanes |
| Ten imports | Public ten-entry pack plus private ten-entry collection, actual decoder playback | Four concurrent streams; roughly 30 seconds total in public pack, mono 16 kHz ADPCM |
| Loop controls | Four level knobs per bank, start/stop, queued rhythmic launch, metadata view | Imported recordings occupy the existing user sample slots |
| Independent key/tempo preparation | ffmpeg offline conversion; decoded sine frequency and duration assertions | CLI preparation, not device-side stretching; browser only trims/normalizes/fades |
| Follow Key/BPM | Metadata match/refusal/fade checked | Exact prepared-target guard; unavailable keys/tempos cannot be regenerated on device |
| Seven sound-shaping sections | Shape, EQ, Drive, Modulation, Grain, Delay, Reverb | EQ/Drive/Mod/Grain shared master; original Delay/Reverb shared sends |
| Stereo/mono effects | Bypass, zero-width, four modulation types and bounded grain feedback host tests | Effect width changes wet signal; dry stereo is retained |
| Grain In Key | Declared root of exactly one active tonal layer | No arbitrary audio pitch detection; limited to the supported reference |
| Save and backup | Versioned 48-byte extension, project size check, roundtrip; actual flash save/load | Layers resume stopped; song backup and sample/collection backup are separate |
| Companion editor | Existing editor extended; browser import plus Python/JS pack compatibility | Actual firmware USB protocol replay verifies uploaded sample bytes; physical WebMIDI remains unchecked |
| Update/recovery | Original package and loader pathways retained | No physical update/recovery attempt; not a hardware-ready claim |
| Ownership and licensing | Fork/branch, upstream remote, licenses, public asset attribution | Personal Desktop derivatives stay in ignored private directory |

## Tests and evidence

`tests/run_lucas.sh` runs sanitizer-backed integration tests, pack tests and browser-code pack tests. Existing engine shift/overflow idioms are excluded from those two specific UBSan checks; other undefined-behavior checks and ASan remain active. Dedicated tests enable Lucas FM; legacy upstream goldens intentionally retain upstream defaults. No audio golden or target-budget baseline was rewritten.

Native replay uses actual compiled instructions, timed LCD SPI, captured audio, RAM diagnostics and fresh flash. Screenshots in `screenshots/` come from that replay. `evidence/` contains compact measurements and logs. Standard workload: four imported layers, eight held analog voices, repeated channel-10 drums, active EQ/drive/modulation/grain and USB audio. The extreme workload changes both pitched tracks to the heavy VOICE engine and maximizes effects. See the measured table below for what passes and what yields.

The final Linux run completed: **182 golden renders unchanged, zero health failures, zero voice/routing failures, zero crashes**. The full upstream suite is **not entirely green**. The unchanged baseline already exceeded four static instruction-cost budgets (sample, legacy grain, drums and audio interrupt) and had a missing host-test include. The include is fixed. Sample decode refactoring increases its static cost from 593 to 662 versus an old 126 budget; the other three budget overruns remain. These static counters are distinct from measured audio deadlines. The old editor identity assertion was extended to recognize FM-1_801; complete Node 22 editor tests pass. Do not interpret a generated artifact as a passing full CI run.

## Measured release results

Compiled source: `334dfb74c2e010879760981a1bec67f3da3b91b6`. Firmware SHA-256: `7b04f7627cc59b6f18ff76e627c25c0d7904e77313515b987609986226a158ee`. [Build/test run](https://github.com/lucasm3110/sloop-fm1/actions/runs/43561087862).

| Actual native-emulator workload | Clock | Maximum audio block | Final CPU estimate | Missed deadlines | Pitched voices retained |
|---|---:|---:|---:|---:|---:|
| Unchanged SLOOP idle, 6 s | 100 MHz | 2,574 µs | 30.86% | 1 at boot | 0 |
| Lucas idle, 6 s | 100 MHz | 2,869 µs | 35.55% | 1 at boot | 0 |
| Eight analog notes, four imports, drums, new effects and USB, 33 s | 100 MHz | 8,160 µs | 72.66% after shedding | 694 | 1 of 8 — unsupported |
| Same analog workload | 240 MHz | 4,741 µs | 70.70% | 0 | 8 of 8; effects remain active |
| Seven TALKBOX/VOICE notes, four imports, maximum effects and USB, 33 s | 240 MHz | 5,179 µs | 78.52% | 0 | 7 of 7; new effects paused visibly |
| Eight TALKBOX/VOICE notes, otherwise identical | 240 MHz | 5,179 µs | 78.91% after shedding | 0 | 7 of 8 — overload warning |

The audio deadline is 5,804 µs per 256-frame block. Maximum block time and the final smoothed CPU reading are different measurements; neither alone proves every possible preset combination is safe. The 240 MHz analog workload processed 1,686 UI frames in 33 s; the seven-VOICE case processed 1,520. The idle comparison includes the startup period and its single initial late counter in both builds. Strict-mode timing is a model, not an oscilloscope measurement on the FM-1.

| Storage/resource | Baseline | Lucas | Available |
|---|---:|---:|---:|
| Application binary | 571,820 B | 579,644 B | 581,564 B |
| Data + BSS | 59,340 B | 85,484 B | 98,304 B |
| Shared pool | 333,900 B | 333,900 B | 344,064 B |
| Low RAM | 22,772 B | 22,772 B | Original linker region retained |
| Project record | 3,816 B | 3,816 B | 3,840 B |

Application flash has only 1,920 bytes of remaining space: future features need size discipline. Four ADPCM streams have dedicated decode states rather than consuming the eight pitched voices. Public starter audio is approximately 30.32 seconds / 239 KiB; the private collection is approximately 24.02 seconds with one slot reserved for its compact drum kit.

Additional release checks: nine actual LCD destinations, recorded section reload with audible output, 961 emulated USB commands transferring all four sample slots byte-for-byte, and personal USR1 kit selection retained after a cold emulator restart. The six pack tests and dedicated host integration tests pass locally. Linux CI passes the same integration tests, with the ffmpeg-dependent pitch/duration test explicitly skipped there; it passed locally with ffmpeg. Full upstream results and the four preserved static-budget failures are documented above.

## Material compromises

- Four simultaneous short mono imports, not ten simultaneous or long recordings. Public collection uses all four sample slots; personal collection reserves USR1 for the drum kit.
- Key/tempo changes require preparing an appropriate pack on the computer. Browser import does not perform that transformation. Loop boundaries use short fades, not adjustable overlap crossfades.
- The new shared effects pause under heavy load, with a visible warning and a short fade. If the underlying instruments still exceed the deadline reserve, the original voice-shedding fallback remains and is visibly indicated. No promise of all engines/effects at maximum without resource limits.
- The 100 MHz heavy workload misses deadlines. It is not supported as a smooth full-layer workload. The pinned emulator README documents 240 MHz as the chip's clock and its native default. These runs use that documented setting; hardware equivalence still needs physical measurement. See [emulator README](https://github.com/joakimeriksson/fm1-emu/blob/883e10f8b37d25dbac1786cc6370f459a8f3d4a4/README.md).
- This is emulator-validated software. It is not yet certified for unattended live performance or recovery on a physical device.

## Physical checks still required

1. Back up existing songs and user sample slots with the original editor; verify recovery access before any authorized update.
2. Check all encoders, buttons, key LEDs, master volume, headphone/output levels and held-note release on an actual FM-1.
3. Check USB audio recording and physical editor sample transfer, disconnect/reconnect, and sample/song backup restoration.
4. Check cold boot, save/reload, power interruption behavior and update/recovery compatibility.
5. Repeat maximum-load testing while listening for clicks/dropouts, then run a long performance session and watch the CPU warning.
6. Test FM-1 audio output into Quad Cortex at conservative input levels. An audio cable does not synchronize tempo; MIDI to QC is not required.
