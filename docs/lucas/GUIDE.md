# Lucas FM

A separate GPL-3.0 firmware fork of SLOOP for the M-VAVE FM-1. This repository is unrelated to TeleNeuMu or Geara. The shipping package is actual JieLi firmware, exercised in the native FM-1 emulator. Physical hardware has **not** been flashed or validated.

## Start playing

On this Mac, double-click `Launch Lucas FM.command` in the LucasFM folder. It opens the real native emulator with your local ten-sound collection and USR1 personal drum kit already installed. It does not connect to physical MIDI devices. `Open Lucas Editor.command` opens the companion editor in Chrome on localhost. Closing its Terminal window stops the local web server.

For another computer, use the source/build instructions or open `dist/lucas-fm-0.3.fwsc` in the hosted emulator's **Open .fwsc…** control. This package already contains the ten public imports. Choose **240 MHz**. The ZIP also includes the transferable `.lucaspack` for the companion editor. On physical hardware, the retained update loader preserves sample slots; transfer the collection separately through the editor.

The initial song is D major, 72 BPM, with Warm Pad, Sub Bass, Rhodes and an editable 808 beat. Press PLAY to hear the drum machine immediately; existing autosaves retain their own patterns. The first three tracks share eight pitched voices; the fourth uses SLOOP's separate drum engine. These are starting sounds: choose any retained synth engine/preset on tracks 1–3. Track 4 remains the dedicated drum track.

Press **SCL** for the scale and playing guide. Its four knobs choose key, scale, BPM and PIANO / NOTES / CHORD. In D major, the seven triads are D, Em, F# minor, G, A, Bm and C# diminished. White keys follow scale degrees in NOTES/CHORD; PIANO keeps all chromatic notes playable. EDIT on this page toggles Hold; changing part, key, mode or octave releases live held notes. Live key changes do not transpose already-recorded sequence notes.

## Controls that match their names

There are seven rotary encoders, no encoder pushes, one master potentiometer, 14 buttons and 27 note keys. The LCD is 240 × 240 and is not a touchscreen.

| Printed control | Everyday behavior |
|---|---|
| SELECT | Select a part. On Home, turn beyond part 4 to select LOOPS, then press EDIT. In Loops select a clip; in Save select an action. |
| PRESETS | Home: all factory/favorite/user sounds. Sound and shaping pages: presets in the current engine. Drums: kits. FX: effect presets or modulation/pitch variations. Loops: banks of four clips. Sequence: stored sections. Save: destination section. |
| ALGORITHM | Choose the selected sound's engine; drums choose a kit. FX: processor. LFO: waveform. ARP: pattern, including OFF. Loops: FREE / KEY / BPM / K+B matching. |
| KNOB 1–4 | Follow the four aligned screen cells. On Home they directly control Pad, Bass, Melody and Drums levels. |
| MASTER | Overall physical output volume. |

| Button | Destination or action |
|---|---|
| HOME | Always return to the four-part overview. |
| EDIT | Home: open the selected sound or Loops. Sound: edit engine parameters; repeat for the second bank and back to the browser. On drums, open the retained drum editor. |
| SCL | Scale, key, tempo and chord/piano guide. EDIT here toggles Hold. |
| ENV | Actual attack, decay, sustain and release, with a responsive envelope drawing. |
| LFO | Actual speed and pitch/filter/level modulation depths. ALGORITHM chooses the waveform. Zero depth means no modulation. |
| ARP | Actual arpeggiator: ALGORITHM chooses OFF or a pattern; knobs set rate, octaves, gate and Hold. |
| FX | Effects. ALGORITHM selects the processor, PRESETS loads settings/variations, EDIT bypasses/restores. FX again flips extra EQ/Grain controls. |
| GLO | Mixer: selected part's level, pan, mute and solo. SELECT chooses the part. |
| SEQ | Note/drum sequence overview. Repeat for the retained detailed editor; EDIT on the overview undoes/redoes. |
| SAVE | Save/load sections or enter advanced system controls. SELECT chooses the action, PRESETS chooses A–D, EDIT twice confirms. Changing the destination cancels pending confirmation. |
| PLAY | Sequence start/stop everywhere. Stopping also fades imported layers and releases held notes. |
| REC | Arm recording when stopped; record/overdub while running. |
| OCT− / OCT+ | Keyboard octave. On Loops, previous/next clip bank. |

Sound selection is immediate. Turning ALGORITHM changes the engine and loads its first sound, so PRESETS can browse from there. Sound's four knobs are Level, Tone, Reverb and Keys; on drums the fourth knob is Delay. Level and send readouts on Sound/Home/Mix use percentages. Engine-specific edits show the engine's own labels and units. ENV/LFO/ARP apply to pitched parts; when Drums is selected those pages clearly direct you to a synth part or SEQ instead of presenting inactive synth controls.

The four colors stay attached to parts everywhere. Footer cells align with the four physical knobs. The amber dot means changes since the last working-song autosave. Muted parts dim; transport and record state remain visible. The retained advanced SLOOP editors keep their original deeper control mappings; HOME always returns to Lucas.

## Loops in three actions

1. HOME, turn SELECT to LOOPS, press EDIT.
2. Turn SELECT to a clip; EDIT starts/stops it.
3. Use the four knobs for the four visible clip levels; PRESETS changes banks.

ALGORITHM chooses the matching rule: FREE preserves the imported audio, KEY requires a prepared matching key, BPM a prepared matching tempo, K+B both. Modes needing missing metadata are skipped. A mismatch is visibly marked; this does not regenerate audio on the device. Texture recordings normally stay FREE. Part selection persists across instrument pages; the chosen bank determines which clips are visible; leaving the screen does not stop it. The library holds ten imports with four simultaneous layers.

Autosave waits for stopped transport, no active voices/layers and an idle panel. A named section save can coexist with pending working-song changes. Stop playback and Hold, stop free loops, and let releases finish before manual save. Sections load only after confirmation in Save; the sequence overview's PRESETS retains the existing live section-switching behavior. Audio collections and songs are separate backups.

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

Home → Loops opens the library. SELECT chooses an entry, EDIT starts/stops it, and K1–K4 adjust the four visible entry levels. OCT changes banks. PLAY, STOP, WAIT and FADE identify state. A fifth concurrent request reports `4 LAYERS BUSY`; it does not steal a held synth voice. Textures loop at their natural pitch/speed. Rhythmic clips start/stop at the next transport bar when the sequencer is running; when stopped they start immediately. Transport stop fades all layers. Free layers can also be stopped individually without starting the transport.

The clip's prepared key and BPM appear beside its name when present. ALGORITHM chooses FREE, KEY, BPM or K+B; missing metadata prevents enabling the corresponding match rule. **Follow is a match guard, not real-time pitch shifting or time stretching.** A mismatched prepared clip is refused or fades out after a song change. With Follow off, it plays the prepared recording unchanged.

The **0.3 play-ready package includes all ten public loops** for a fresh browser/native emulator. The local personal package includes your collection and drum kit. The physical update loader deliberately preserves existing user sample slots: physical installations still need collection transfer through the editor; no physical device has been flashed.

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
| Shape | Selected engine's attack/decay/sustain/release; TONE on Sound is the track filter | Envelope is the sound shape, not an insert effect |
| EQ | Shared master; low cut, bass, mid, treble; page 2 high cut and bypass | EDIT, or page 2 ACTIVE = OFF |
| Drive | Shared master; amount, tone, output | Amount = 0 |
| Modulation | Shared master; PRESETS chooses chorus/vibrato/tremolo/phaser; amount, speed, depth, width | Amount = 0 |
| Glitch / Grain | Shared master; mix, size, delay, scatter; page 2 density, timing randomness, feedback, width; PRESETS chooses Octaves / In Key / Free | Mix = 0 |
| Delay | Selected track send plus shared time division, feedback and tone | Selected send = 0; clear other sends for a fully dry bus |
| Reverb | Selected track send plus shared decay/size and tone | Selected send = 0; clear other sends for a fully dry bus |

Modulation width changes the wet modulation phase spread, preserving the dry source's stereo image. At zero width the effect still works and identical left/right inputs remain identical. The stereo effect is not made by polarity inversion. Grain is an original four-grain processor with a short bounded buffer, triangular windows and bounded feedback. Scatter zero means no randomized pitch. **In Key only takes audio from exactly one active tonal loop with a declared root**; it does not analyze arbitrary recordings or correct every note in a polyphonic pad. Multiple tonal loops or an unknown root provide no new In Key grain input. Octaves/Free process the master mix. Delay/reverb reuse the existing shared buses and limiter. Reverb size and decay are coupled by the retained engine; they are not independent controls.

EQ activates when any EQ parameter is edited. The screen explicitly shows ACTIVE or BYPASSED; EDIT toggles the selected effect. ENV opens the separate envelope page. FX flips the extra EQ/Grain page. Grain MIX is now a dry/wet crossfade with compensated grain level, making the effect obvious as MIX rises. Parameter changes retry processing after a CPU pause. Large effect changes are smoothed where relevant. If two audio buffers exceed about 85% load, the new shared EQ, Drive, Modulation and Grain processors yield first. Their last contribution fades to dry over 64 samples, preserving held notes. A persistent `NEW FX PAUSED: CPU` message appears; press FX to retry after reducing the load. Existing delay/reverb buses and the limiter remain. After that first reduction, the last-resort threshold uses 92% of the audio deadline, including USB service time. If the retained engine alone still overloads, its last-resort voice shedding remains and the screen reports `VOICE LIMIT: CPU`. The 100 MHz emulator setting cannot sustain the maximum tested load. See 0.3.md for current measurements and VALIDATION.md for historical measurements at both 100 and the device's nominal 240 MHz.

## Connections and updates

Use the FM-1 audio output into the Quad Cortex audio input, then QC effects/headphones/PA. That audio cable does not synchronize tempo. USB audio recording remains the existing stereo 44.1 kHz path. This build does not add a microphone, analog recording input, SD card or real-time long-recording storage.

Loader/update/recovery support and the FUN6 project layout are retained. Lucas settings use a versioned extension in the drum track's unused synth parameters; legacy songs get Lucas defaults. Old SLOOP can read the surrounding song format but does not understand Lucas effects or loop metadata. Do not edit those reserved drum synth parameters through legacy tools if retaining Lucas settings matters. The import extension is documented in DEVELOPMENT.md. No physical firmware update was performed during this work.
