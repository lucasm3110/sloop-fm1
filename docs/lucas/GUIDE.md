# Lucas FM

A separate GPL-3.0 firmware fork of SLOOP for the M-VAVE FM-1. This repository is unrelated to TeleNeuMu or Geara. The shipping package is actual JieLi firmware, exercised in the native FM-1 emulator. Physical hardware has **not** been flashed or validated.

## Start playing

The initial song is D major, 72 BPM, with Warm Pad, Sub Bass, Rhodes and drums. The first three tracks share eight pitched voices; the fourth uses SLOOP's separate drum engine. These are starting sounds: choose any retained synth engine/preset on tracks 1–3. Track 4 remains the dedicated drum track.

Press ARP to open PLAY. Consecutive white keys walk the selected scale; physical C4 is degree 1, D4 is degree 2, and so on, with lower and higher octaves on either side. In D major the seven triads are D, Em, F# minor, G, A, Bm and C# diminished. The wheel shows the root, quality and chord tones. Black keys are silent in the everyday number/chord modes. The deeper SLOOP interface retains its optional chord modifiers.

K4 chooses PIANO, NUM (single scale notes) or CHORD (diatonic triads). PIANO preserves chromatic playing, including out-of-scale notes; the screen and single-color key lights guide the scale. ARP again toggles chord hold. A new chord replaces the held chord; toggling hold off releases it. Changing track, key, mode or octave releases the affected live notes. Key changes affect live mapping and matching of prepared loops; they do not transpose recorded sequence notes.

## Physical controls

There are seven rotary encoders, **no encoder pushes**, one master potentiometer, 14 buttons and 27 note keys. The LCD is 240 × 240 and is not a touchscreen.

| Control | Function |
|---|---|
| HOME | Return directly to the performance overview, including from deeper SLOOP pages |
| SELECT on Home | Choose PLAY, SOUND, SEQUENCE, LOOPS or MIX; EDIT opens the choice |
| ALGORITHM | Select Pad, Bass, Melody or Drums; selection follows between pages |
| ARP | Open PLAY; on PLAY, toggle hold |
| EDIT | Open SOUND; on Home open destination; on Loops toggle selected sound; on Sequence undo/redo |
| PRESETS on SOUND | Browse all retained factory and user presets; on Drums, choose a kit |
| SCL / SEL | Key & Tempo; tap again to return Home |
| SEQ | Sequence overview; tap again for the retained detailed note/drum editor |
| LFO | Loop library; tap again for selected loop's metadata and Follow settings; again returns library |
| GLO | Mixer for selected track |
| ENV | Shape envelope for selected track |
| FX | Effects; SELECT chooses section; FX flips the EQ or Grain page |
| SAVE | SELECT chooses save section, load section or system/update; K1 chooses A–D; EDIT twice confirms save/load |
| PLAY | Sequence start/stop; stopping also fades imported layers and releases held notes |
| REC | Arm recording when stopped; toggle recording/overdub when running |
| OCT− / OCT+ | Live octave; on Loops switch four-entry bank; on Effects flip page |
| MASTER | Physical output volume |

The four right-hand knobs always follow their visible footer. On Home and Key: root, scale, BPM, keyboard mode. On Play/Sound: track level, track filter/tone, reverb send, keyboard mode. “REVERB” is a send, not an undocumented macro. On Mix: level, pan, mute and solo. Turn mute/solo clockwise on, counterclockwise off. There is no required hold combination for everyday navigation. HOME is the universal way back.

The `*` marks differences from the last **working-song autosave**, not merely from the current named section. Autosave waits for stopped transport, no active voices/layers, an idle panel, and the existing minimum write interval. A manually saved section can coexist with a pending working-song autosave. Manual save refuses active sounds; stop playback, turn hold off, stop free loops, and let releases finish. Named sections load only after confirmation. The deep editor retains erase/undo and the existing recovery path.

## Sequence

K1 sets 1–4 bars, K2 selects first-note start or one-bar count-in, K3 selects free timing or 1/16 quantization, and K4 selects click. Press REC, then play a note to start; with COUNT, press PLAY for four counted beats. The header distinguishes ARM, COUNT, REC and PLAY. REC again ends recording while playback continues. EDIT swaps undo/redo. PRESETS selects an existing A–D section; empty sections are identified. SEQ again opens SLOOP's detailed step editor for note lengths, drums, locks and other retained features. GLO provides track mute and solo.

## Ten imported sounds

The supplied public **Park in D** collection contains six CC0 field-recording excerpts, three pads rendered from SLOOP, and a one-bar CC0 drum arrangement. These are decoded imports, not ten links to factory presets.

| Resource | Limit |
|---|---|
| Library | Ten named imported entries per collection |
| Resident storage | Four shared 80 KiB sample slots, including headers and metadata |
| Simultaneous imported layers | Four, with separate ADPCM decoder state |
| Public starter | Approximately 30 seconds total at mono 16 kHz / 4-bit IMA ADPCM |
| Private desktop collection | Ten shorter loops in three slots, with USR1 reserved for a personal drum kit |

LFO opens the library. SELECT chooses an entry, EDIT starts/stops it, and K1–K4 adjust the four visible entry levels. OCT changes banks. PLAY, STOP, WAIT and FADE identify state. A fifth concurrent request reports `4 LAYERS BUSY`; it does not steal a held synth voice. Textures loop at their natural pitch/speed. Rhythmic clips start/stop at the next transport bar when the sequencer is running; when stopped they start immediately. Transport stop fades all layers. Free layers can also be stopped individually without starting the transport.

Tap LFO again for the selected sound's prepared key/BPM. K2 and K3 explicitly enable/disable Follow Key and Follow BPM. Missing metadata prevents enabling that option. **Follow is a match guard, not real-time pitch shifting or time stretching.** A mismatched prepared clip is refused or fades out after a song change. With Follow off, it plays the prepared recording unchanged.

The default package does not contain the imported audio. Transfer a `.lucaspack` through the editor, or use the supplied complete **native emulator** flash image. Firmware and collections are separate so future updates can preserve sample storage.

## Prepare and back up sounds

Open `web/editor.html` through localhost in Chrome/Edge, connect Lucas FM, and use Samples → **Lucas FM · Imported loop collection**. Add up to ten recordings; set names, type, trim times, declared root, BPM/bar length and Follow defaults. Prepare shows actual slot consumption before transfer. Save collection backup downloads a `.lucaspack`; Open prepared collection restores it. Transfer replaces the collection's slots and preserves explicitly reserved slots. Existing instruments/drum kits share those slots; use the original editor's backup tools before replacing them.

Browser preparation trims, normalizes and fades boundaries. It **does not** change pitch/tempo. For independently matched target-song audio, install ffmpeg and run:

```sh
python3 tools/lucas_pack.py assets/loops-cc0/collection.json my-pack.lucaspack --key E --bpm 80
```

The manifest format is demonstrated in `assets/loops-cc0/collection.json`. Source filenames are relative to that manifest. `type` is texture, tonal or rhythmic; `root` is a pitch name, `bpm` is source tempo, `bars` is whole loop bars. `follow_key`/`follow_bpm` override defaults. Optional `slots: [1,2,3]` reserves zero-based slot 0 for a drum kit. WAV/MP3/other ffmpeg-readable recordings work. Choose `--rate 12000`, `16000` or `22050` to trade bandwidth against duration. If the collection will not fit, preparation fails before transfer.

Offline pitch change uses resampling followed by ffmpeg's WSOLA `atempo` to restore/change duration independently. This is useful preparation for short clips, not studio-quality polyphonic source separation. The output identifies its prepared key/BPM. Boundaries have a short fade to zero at both ends; there is no adjustable overlap crossfade in this version. Long environmental recordings must be shortened to fit.

Personal assets, when prepared, live in the ignored `private/` directory. Install `personal-d72.lucaspack`, then open `personal-drums.zip` in the editor's Drum Kit page, send to USR1 and select USR1 on track 4. The ten-hit compact kit leaves unused lanes silent. Reimport full originals if different trims are preferred. The private files are never part of the public repository or release.

Use the existing Song/Projects and device-backup functions for notes, settings and samples; use `.lucaspack` for collection metadata/audio. Back up both song data and sample data. Sample flash contents are independent of the section file. Collection levels and Follow overrides are saved with the song; layers start stopped after loading, preventing unexpected audio.

## Effects

| Section | Scope and controls | Bypass |
|---|---|---|
| Shape | Selected engine's attack/decay/sustain/release; TONE on Play/Sound is the track filter | Envelope is the sound shape, not an insert effect |
| EQ | Shared master; low cut, bass, mid, treble; page 2 high cut and bypass | Page 2 BYPASS = 1 |
| Drive | Shared master; amount, tone, output | Amount = 0 |
| Modulation | Shared master; PRESETS chooses chorus/vibrato/tremolo/phaser; amount, speed, depth, width | Amount = 0 |
| Glitch / Grain | Shared master; mix, size, delay, scatter; page 2 density, timing randomness, feedback, width; PRESETS chooses Octaves / In Key / Free | Mix = 0 |
| Delay | Selected track send plus shared time division, feedback and tone | Selected send = 0; clear other sends for a fully dry bus |
| Reverb | Selected track send plus shared decay/size and tone | Selected send = 0; clear other sends for a fully dry bus |

Modulation width changes the wet modulation phase spread, preserving the dry source's stereo image. At zero width the effect still works and identical left/right inputs remain identical. The stereo effect is not made by polarity inversion. Grain is an original four-grain processor with a short bounded buffer, triangular windows and bounded feedback. Scatter zero means no randomized pitch. **In Key only takes audio from exactly one active tonal loop with a declared root**; it does not analyze arbitrary recordings or correct every note in a polyphonic pad. Multiple tonal loops or an unknown root provide no new In Key grain input. Octaves/Free process the master mix. Delay/reverb reuse the existing shared buses and limiter. Reverb size and decay are coupled by the retained engine; they are not independent controls.

Large effect changes are smoothed where relevant. The original overload voice shedding remains as last-resort protection; `LOAD` warns above roughly 85% audio CPU on non-Home pages. The 100 MHz emulator setting cannot sustain the maximum tested load. See VALIDATION.md for measurements at both 100 and the device's nominal 240 MHz.

## Connections and updates

Use the FM-1 audio output into the Quad Cortex audio input, then QC effects/headphones/PA. That audio cable does not synchronize tempo. USB audio recording remains the existing stereo 44.1 kHz path. This build does not add a microphone, analog recording input, SD card or real-time long-recording storage.

Loader/update/recovery support and the FUN6 project layout are retained. Lucas settings use a versioned extension in the drum track's unused synth parameters; legacy songs get Lucas defaults. Old SLOOP can read the surrounding song format but does not understand Lucas effects or loop metadata. Do not edit those reserved drum synth parameters through legacy tools if retaining Lucas settings matters. The import extension is documented in DEVELOPMENT.md. No physical firmware update was performed during this work.
