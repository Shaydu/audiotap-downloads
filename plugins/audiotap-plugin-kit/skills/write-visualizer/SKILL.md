---
name: write-visualizer
description: Use when someone asks for a visualizer for AudioTap (or AudioFly) — a music-reactive picture, spectrum, waveform, meter or light show. Turns their description into an ISF 2.0 shader file that AudioTap picks up from its plugins folder without a restart.
---

# Write an AudioTap visualizer

An AudioTap visualizer is one ISF 2.0 shader file (`.fs`). AudioTap feeds it the music's spectrum and waveform, gives
each of its settings a control, and reloads it whenever the file changes. No compiler, signing or Apple account is
needed.

**Read `isf-host-profile.md` (next to this file) before writing anything.** It is the contract: what AudioTap
supplies, what it rejects, and the reason texts it shows. The ISF file format itself is at
https://github.com/mrRay/ISF_Spec. `examples/LevelBars.fs` is a complete, working example.

## Steps

1. **Understand the request.** Get from the contributor: what the picture looks like, what in the music drives it
   (bass, overall level, the waveform, the whole spectrum), and which qualities they'd like to adjust. If something
   essential is missing, ask once; otherwise choose sensibly and say what you chose.
2. **Pick a file name** in CamelCase from the visualizer's name (`NebulaPulse.fs` shows as "Nebula Pulse"). Check the
   plugins folder for a file with the same name first.
3. **Write the header.** `"ISFVSN": "2"`, a `DESCRIPTION` under 60 characters, `CREDIT` with the contributor's name
   if they gave one, and `INPUTS`:
   - `audioFFT` for anything driven by frequencies or loudness; `audio` for oscilloscope-style pictures. Set `MAX`
     to the resolution you need (fewer columns are cheaper and already averaged).
   - one input per adjustable quality, each with `LABEL`, and `MIN`/`MAX`/`DEFAULT` where the profile says so.
     Four to six controls is plenty.
   - `PASSES` with a `PERSISTENT` buffer if you need smoothing, trails or feedback.
4. **Write the shader** following the profile's portability rules: `gl_FragColor`, `IMG_NORM_PIXEL`, constant loop
   bounds, every variable initialised. Spectrum columns are log-spaced (bass on the left) and values are 0…1 from
   −96…0 dBFS; the waveform is 0…1 with 0.5 as silence. Don't draw text: AudioTap draws the track caption itself.
5. **Check it.** If `/Applications/AudioTap.app` exists, run
   `/Applications/AudioTap.app/Contents/MacOS/AudioTap --check-plugin <file>.fs`. Fix whatever it reports and run it
   again until it prints `OK`. If AudioTap isn't installed, re-read the shader against the profile line by line
   instead, and tell the contributor it hasn't been machine-checked.
6. **Install it.** Copy the file (and any `.vs` or images) to
   `~/Library/Application Support/AudioTap/Plugins/Visualizers/`, creating the folder if needed. Don't overwrite a
   different visualizer's file.
7. **Tell the contributor** in a short checklist:
   - where it is: **Visualizer ▾ →** its name, with the **Shader** badge;
   - what each control in the **Controls** panel does;
   - that saving the file again reloads it in place, so they can tweak it live;
   - if it shows a red dot in **Visualizer ▾**, the reason text there says what to fix.

## What this can't do

- No camera, video or other image input (only images bundled with the shader through `IMPORTED`).
- No text; AudioTap draws the now-playing caption.
- No sound processing: for an audio effect, use the `write-dsp-plugin` skill.
