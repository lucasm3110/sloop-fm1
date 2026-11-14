# Lucas Loops 0.4

Four layers: **pad, bass, melody, rhythm**. Each plays a finished loop. Two screens: your mix and the loop chooser.

## Try it

Double-click **Open Lucas Loops.command** in the project folder. Click **Start listening**, then **Play**. Select bass, melody or rhythm and press **Layer on / off** to bring it in. Drag each colored knob up or down to mix. The pad starts enabled; other parts start muted.

Choose a layer, use **PRESETS − / +**, then **Load loop**. Home cancels browsing. A replacement joins at the next bar after its short fade; a very late choice waits one extra bar. All loops share the same clock.

Stop and press **Save**. The browser keeps your sounds, levels and enabled layers on this computer. Playback always starts stopped. Saves belong to the exact firmware package and browser address; public and personal builds have separate saves.

## FM-1 controls

| Control | Action |
| --- | --- |
| SELECT encoder | Select pad / bass / melody / rhythm |
| PRESETS encoder | Browse loops for that layer |
| EDIT | Toggle layer, or load the chosen loop |
| HOME | Return to the mix and cancel an uncommitted choice |
| PLAY | Start / stop all four loops |
| Knobs 1–4 | Pad / bass / melody / rhythm level, on either screen |
| MASTER | Overall output volume |
| SAVE, stopped | Save the working scene |

Keyboard in the browser: Space plays/stops, left/right selects a layer, up/down browses sounds, Enter loads/toggles, Escape returns home. Focus a volume knob and use arrows for fine adjustment.

## Included loops

- Pad: warm pad, soft air, glass tide.
- Bass: sub bass, round bass.
- Melody: soft keys, tiny bells, slow pluck.
- Rhythm: soft beat, light steps.

All ten are one-bar loops at **72 BPM, D major**. Tonal loops were rendered from SLOOP engines. The public rhythm loops use CC0 samples; the personal local package uses your Desktop kit. The rhythm layer plays finished loops; there is no drum machine or sequencer editor. Tempo and key are fixed in this release. The old synth, algorithm and effects pages are absent from this interface.

## Files

`dist/lucas-fm-0.4.fwsc` includes the public collection for the emulator. `dist/lucas-fm-0.4-update.fwsc` is firmware only and does not install the collection. `private/lucas-fm-0.4-personal.fwsc` contains your personal rhythms and is excluded from Git. The browser launcher prefers that personal package when it exists.

Physical FM-1 hardware has not been flashed or tested. Browser and native emulation run the compiled firmware; they do not establish physical-device compatibility.
