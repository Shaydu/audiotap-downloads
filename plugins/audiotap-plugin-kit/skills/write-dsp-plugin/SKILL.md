---
name: write-dsp-plugin
description: Use when someone asks for an audio effect for AudioTap (or AudioFly) — a plugin that changes the sound, such as gain, EQ, compressor, saturation, filter, delay or reverb. Writes it as a CLAP audio effect in C, builds a universal macOS .clap, signs it, validates it with clap-validator, and installs it where AudioTap picks it up.
---

# Write an AudioTap audio effect

An AudioTap audio effect is a standard [CLAP](https://github.com/free-audio/clap) plugin: a `.clap` bundle that
processes the sound AudioTap plays. AudioTap gives each of its parameters a slider, switch or menu, and picks it up
from the standard CLAP folder without a restart.

**Read `clap-host-profile.md` (next to this file) before writing anything.** It is the contract: what AudioTap
provides, what it rejects, the real-time rules for the audio code, and the reason texts it shows.
`examples/stereo-gain/` is a complete, working effect (plain C, CMake): start from it.

## Before you start

You need a Mac with the Xcode command line tools (`xcode-select --install`), `cmake` and `git`. Get
[`clap-validator`](https://github.com/free-audio/clap-validator/releases) too; without it you can't machine-check the
plugin. If something is missing, tell the contributor what to install rather than skipping the step.

## Steps

1. **Understand the request.** Get from the contributor: what the effect does to the sound, and which qualities they
   want to adjust (amount, frequency, speed, mix). If something essential is missing, ask once; otherwise choose
   sensibly and say what you chose.
2. **Copy the example.** Copy `examples/stereo-gain/` to a new folder outside the skill (the contributor's own
   projects folder). Rename the source file, the CMake target and `OUTPUT_NAME`, the bundle name in `Info.plist.in`,
   and `CMakeLists.txt`'s `project()`.
3. **Give it its own identity.** In the descriptor: a new reverse-DNS `id` (`org.<their-name>.<effect>`; two plugins
   with one id can't both load), `name`, `vendor`, `version`, a one-line `description`, and the `audio-effect`
   feature. Keep one main stereo input and one main stereo output unless the contributor needs mono.
4. **Expose the adjustable qualities as parameters.** One CLAP parameter each, with `min_value`, `max_value`,
   `default_value`, a readable `value_to_text` ("−6.0 dB", "250 ms") and `text_to_value`. Use
   `CLAP_PARAM_IS_STEPPED` for on/off and choice parameters (they become switches and menus). Keep the list short.
   Save and load every parameter in `state`, and request a values rescan after loading, as the example does.
5. **Write the audio code** in `process`, following the profile's real-time rules: no allocation, locks, file or
   network access, or logging; handle any `frames_count` from 1 to 8192; apply parameter events at their sample
   positions; smooth every parameter that changes the level or a filter so it never clicks; read each input sample
   before writing the output (the buffers may be the same). Allocate everything you need in `activate`, sized for
   8192 frames. Keep the output below 0 dBFS yourself: AudioTap's limiter at −1 dBFS is a safety net, not a design
   tool.
6. **Build, sign and validate.** From the plugin's folder run `./build.sh`. It builds a universal (arm64 + x86_64)
   bundle with CMake, signs it ad-hoc (`codesign -s - --force --deep`), and runs `clap-validator validate` when it is
   installed. Fix everything the validator reports and run it again until it reports no failures. The first build
   downloads the CLAP headers at the pinned tag, so it needs a network connection.
7. **Install it.** Copy the `.clap` bundle (the whole folder) to `~/Library/Audio/Plug-Ins/CLAP/`, creating the folder
   if needed. Don't overwrite a different plugin's bundle. Because it is ad-hoc signed, AudioTap only loads it with
   **Allow plugins I built myself** on; if you copy it after signing, run `codesign` again if anything changed.
8. **Build for Linux, if asked.** The same source builds for the planned AudioFly server on Linux: run
   `./build.sh` on a Linux machine. It builds a `.clap` shared object from the same `CMakeLists.txt` and source,
   and validates it with `clap-validator` if installed. Don't add macOS-only code to the shared source.
9. **Tell the contributor** with the checklist below.

## Checklist for the contributor

- Where it is: AudioTap's SignalChain **Plugins** tab, under the plugin's name. It appears within a couple of seconds
  of installing; restart AudioTap only to pick up a replaced file.
- Turn on **Allow plugins I built myself** (Visualizer menu) first, or the plugin is listed as "Signed by an unknown
  developer".
- Turn the plugin on, **starting at a low volume**: a new effect can be much louder than the music.
- What each control does (name, range and what you'll hear), and that **Reset** returns them to their defaults.
- If the plugin is listed with a reason instead of an on switch, the reason text says what to fix. Common ones are in
  the profile's "When a plugin doesn't load" table.
- To report a problem, say what you tried, what happened and which part of the profile misled you, as described in
  CONTRIBUTING.md.

## What this can't do

- No plugin window: AudioTap doesn't open a CLAP plugin's own interface. Every control comes from the parameters.
- No note, MIDI or sidechain input, and no VST3 or Audio Unit plugins.
- No video, and no control over a visualizer: for a picture that moves with the music, use the `write-visualizer`
  skill.
- The AudioFly server doesn't host CLAP plugins yet. The Linux build is the preparation, not a working install.
- It can't make AudioTap trust an unsigned plugin: a plugin that isn't signed is never loaded.
