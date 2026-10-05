# AudioTap CLAP host profile

**Profile version 1** · for CLAP 1.x · applies to AudioTap for macOS

This profile says exactly what AudioTap supports from the [CLAP](https://github.com/free-audio/clap) plugin standard
(CLever Audio Plugin, MIT licence). It doesn't restate the standard: read the CLAP headers for the interface, then
this page for what AudioTap provides and accepts. A plugin that follows both, and passes `clap-validator`, loads in
AudioTap.

## Where CLAP audio effects run

AudioFly does all of its sound processing on its server, and that stays the rule. So for AudioFly a CLAP effect runs
on the server, never in an app.

| Host | CLAP audio effect |
| --- | --- |
| **AudioTap** (macOS) | **Supported** (this profile) |
| **AudioFly server** (Linux) | Planned: the same plugin source, built for Linux, hosted in the server's chain for every stream and cast |
| **AudioFly web, macOS, iPhone, iPad and Apple TV apps** | No plugin runs in an app. The apps play the server's processed sound and will control the plugin's parameters remotely, as they do the built-in stages |

Planned hosts are not built yet. Follow the rules below, and build the same source for Linux (see "Building for
Linux"), and your plugin is ready for them.

## What a plugin is

- A standard CLAP **audio effect**: a `.clap` bundle that exports `clap_entry`, whose plugin factory offers at least
  one plugin with the `audio-effect` feature (`CLAP_PLUGIN_FEATURE_AUDIO_EFFECT`).
- **One main audio input port and one main audio output port**, each **mono or stereo** (`CLAP_PORT_MONO` or
  `CLAP_PORT_STEREO`). Extra ports are ignored. Nothing AudioTap-specific is required.
- Instruments, note effects, and analyzers with no audio output are listed as "Not an audio effect" and not offered.
- **Id:** the descriptor's `id`, in reverse-DNS form (`org.example.my-effect`). Two plugins with the same id: the
  first found wins (your user folder before the system folder) and the other is listed as "Duplicate id".
- **CLAP version:** any `clap_version` with major version 1 that is not newer than the headers AudioTap was built
  with. Build against a recent 1.x release (the example pins one).
- Mono or stereo must match the sound. AudioTap's chain is stereo, so a mono-only plugin is listed as "Needs mono"
  and not run, and a stereo plugin on a mono signal as "Needs stereo". Make stereo your default.

## Extensions

| Extension | Use |
| --- | --- |
| `clap.audio-ports` | **Required.** Describes the ports above |
| `clap.params` | **Required.** Even a plugin with no controls must provide it, with a count of 0 |
| `clap.latency` | Used when present: the latency you report is added to the visualizer delay, so pictures stay in step with sound |
| `clap.state` | Used when present: saved and restored with the plugin's on/off and order (see "State") |
| `clap.tail` | Used when present |
| `clap.render` | Used when present (AudioTap always asks for real-time rendering) |
| `clap.gui` | **Not used.** AudioTap doesn't open a plugin's own window in this version; every plugin gets generic controls built from its parameters |
| Host side: `clap.thread-check`, `clap.log` | AudioTap provides them to the plugin. Use `log` only on the main thread, never in `process` |

A plugin that needs other host extensions should check for them and work without them.

## Where to put a plugin

AudioTap looks in the standard CLAP folders, so a plugin you already use in another host is found too:

| Folder | For |
| --- | --- |
| `~/Library/Audio/Plug-Ins/CLAP/` | Your user account. **Put your plugins here.** AudioTap creates it if it is missing |
| `/Library/Audio/Plug-Ins/CLAP/` | All users on the Mac |
| Each folder in the `CLAP_PATH` environment variable (colon-separated), when set | Anything else |

AudioTap scans each folder and **one level of sub-folders** (vendors often use one). In AudioTap, the **Plugins** tab
of the SignalChain has **Show Plugins Folder**.

- New plugins appear in the Plugins tab within about 2 seconds, without restarting AudioTap.
- A loaded `.clap` is code in memory, so **replacing or removing** one takes effect after a restart; the tab says
  "Restart AudioTap to update".
- Scanning only reads each plugin's descriptor. No plugin instance is created until you turn it on (or it was on when
  you last quit).

## Threading

AudioTap follows CLAP's threading rules:

- **Main thread:** `init`, `activate`, `deactivate`, `destroy`, `params` queries and rescans, and `state` save and
  load. These may allocate and lock.
- **Audio thread:** `start_processing`, `process`, `stop_processing` and `reset`. Nothing here may block.
- `process` is never called at the same time as `activate`, `deactivate` or `state.load` on the same instance.
- When the sound's sample rate or channel count changes, AudioTap deactivates the plugin and activates it again off
  the audio thread, with a short fade. Don't assume the rate stays fixed for the plugin's life.

## Real-time rules for `process`

`process` runs on the audio thread, once per buffer, with a deadline measured in milliseconds. A slow call is heard as
a glitch. Inside `process` (and `start_processing`, `stop_processing`, `reset`, and parameter `flush`):

| Don't | Do instead |
| --- | --- |
| `malloc`, `calloc`, `realloc`, `free`, C++ `new`/`delete`, growing a `std::vector` or `std::string` | Allocate everything in `activate`, sized for the largest buffer (8192 frames), and reuse it |
| Take a mutex, wait on a semaphore, or call anything that might | Pass values in with plain atomics or with CLAP parameter events; never wait for the main thread |
| `printf`, `fprintf`, `NSLog`, writing a file, reading a file, network calls | Keep a counter and report it from the main thread (`on_main_thread` with `host->request_callback`) |
| Call `host->log` | Same: log only from the main thread |
| Call Objective-C or Swift runtime, or anything that retains/releases | Plain C or C++ data and function calls only |
| `sleep`, spin-wait, or any loop whose length depends on data you don't control | Loops with fixed or buffer-sized bounds |
| Compute expensive tables (`sin`, `pow` per sample) from scratch | Compute once in `activate` or when a parameter changes; smooth towards it per sample |
| Change a parameter's effect abruptly | Smooth it (the example uses a 20 ms one-pole filter) so changes never click |

Also:

- **Frames per call:** `frames_count` is between **1** and **8192**. Handle any value, including 1. The buffer size
  can change from call to call.
- **Activation:** AudioTap calls `activate(sample_rate, 1, 8192)` with the sound's sample rate.
- **Parameter events** arrive as `CLAP_EVENT_PARAM_VALUE` in `process`'s `in_events`, each with the sample position
  (`time`) inside the buffer. Apply each at its position, or at the start of the buffer.
- **In place:** the input and output buffers may be the same memory. Read a sample before writing it.
- **Denormals:** AudioTap flushes denormal floating-point numbers on the audio thread while plugins run, but keep
  feedback paths from decaying into them.
- **Return `CLAP_PROCESS_CONTINUE`** (or `CLAP_PROCESS_CONTINUE_IF_NOT_QUIET` or `CLAP_PROCESS_TAIL`). Return
  `CLAP_PROCESS_ERROR` only for an unrecoverable problem: it stops your plugin for the session.

## Controls and parameters

Every plugin gets **generic controls** built from its `params`. The Plugins tab lists found plugins (name, vendor,
version), and for each plugin that is on, a control per parameter:

| Your parameter | AudioTap shows |
| --- | --- |
| Numeric (not stepped) | A **slider** over `min_value` … `max_value`, with its value shown as text from your `value_to_text` |
| `CLAP_PARAM_IS_STEPPED` with range 0…1 | A **switch** |
| `CLAP_PARAM_IS_STEPPED` with more values | A **menu**, with each value's text from `value_to_text` |
| `CLAP_PARAM_IS_HIDDEN` | Not shown |
| `CLAP_PARAM_IS_READONLY` | Not shown |

- The control label is the parameter's `name`; `module` is not shown.
- **Reset** returns every parameter to its `default_value`.
- Always set `min_value`, `max_value` and `default_value`, and implement `value_to_text` so units are readable
  ("−6.0 dB", not "−6").
- Changes you make yourself (`CLAP_PARAM_RESCAN_VALUES`, or output parameter events) show in the tab. After a state
  load, request a values rescan so the controls follow.
- Keep the list short: a handful of parameters is easier to use.
- Mix depth in AudioTap's SignalChain doesn't scale plugins. Offer a dry/wet parameter if your effect needs one.

## Where plugins sit in the signal chain

- Plugins run in one **Plugins** slot **after** AudioTap's built-in stages (Loudness, Environment, Listener) and
  **before** the output safety limiter. They see the corrected signal.
- The slot is called once per buffer with all channels (CLAP processes channels together).
- Several plugins can be on at once, in the order you set (drag the handle). A turned-off plugin is deactivated and
  not called.
- With no plugin on, the sound is bit-identical to AudioTap without plugin support.
- Turning a plugin on or off, reordering, re-activation and auto-bypass crossfade over a short window, so they don't
  click.
- Visualizers are fed before the plugins, **delayed by the total latency your plugins report** so pictures match
  what you hear.

## State

- Each plugin's on/off, order and saved settings persist across relaunch.
- If the plugin has the `state` extension, AudioTap saves and restores its state through it. Otherwise AudioTap
  saves its parameter values and restores them as parameter events.
- `state.save` must be deterministic (no uninitialised bytes), and `state.load` must reject data it doesn't
  recognise by returning false. `clap-validator` checks both.

## Safety

AudioTap protects the listener's ears and speakers from a faulty plugin:

- **Scrubbing:** a plugin's output that contains NaN or ±Inf is replaced by silence for that buffer.
- **Safety limiter:** while any plugin is on, an always-on **limiter with a −1 dBFS ceiling** (and a short look-ahead
  whose latency is counted in the visualizer delay) runs after the Plugins slot. You can't switch it off while plugins
  run. It isn't a substitute for sensible gain: aim for output that stays below 0 dBFS on its own.
- **Auto-bypass:** a plugin is bypassed for the rest of the session, with a short fade, and listed as
  "Stopped: <reason>", when it:
  - outputs NaN or ±Inf;
  - takes more than **50% of the buffer period** (the buffer's duration) in **8 consecutive** buffers;
  - returns `CLAP_PROCESS_ERROR`.
- **Crash guard:** a plugin that crashed AudioTap while loading, activating, or in its first 5 seconds of
  processing is skipped on the next launch until its file changes.

Plugins run inside AudioTap's process. A plugin that crashes takes AudioTap down; the crash guard keeps it from
doing it again.

## Signing and trust

macOS plugins are code, so AudioTap checks the signature **before** it loads a plugin.

| The plugin is | AudioTap |
| --- | --- |
| Signed with a **Developer ID** (and not quarantined) | Loads it by default |
| **Ad-hoc signed** (`codesign -s -`, what you get when you build your own) | Loads it **only** when **Allow plugins I built myself** is on (Visualizer menu); otherwise "Signed by an unknown developer" |
| **Unsigned** | **Never** loads it: "Not signed" |
| **Quarantined** (downloaded from the internet) | Won't load it until you remove the quarantine flag |

The signature covers the whole bundle, so sign after the last file is in place. To sign your own plugin:

```
codesign -s - --force --deep YourPlugin.clap
```

To remove the quarantine flag from a plugin you downloaded and trust:

```
xattr -dr com.apple.quarantine <path to the .clap>
```

Apple silicon Macs refuse to run unsigned code at all, so an unsigned plugin built on one needs at least the ad-hoc
signature above.

## Universal binaries

Build the macOS plugin for **both arm64 and x86_64** (one universal binary). AudioTap runs natively on Apple silicon
and Intel, and a plugin with only one architecture is a plugin that works on half the Macs. In CMake:
`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`. Check with `lipo -archs <bundle>/Contents/MacOS/<name>`.

A macOS `.clap` is a bundle (a folder) with an `Info.plist` and the binary in `Contents/MacOS/`. A single file
renamed to `.clap` is not enough on macOS.

## Checking a plugin

Run [`clap-validator`](https://github.com/free-audio/clap-validator/releases) before installing:

```
clap-validator validate path/to/YourPlugin.clap
```

Fix every failure it reports. AudioTap needs no extra check beyond that and the rules on this page.

## When a plugin doesn't load

The **Plugins** tab lists every plugin file it found. Plugins that didn't load are listed with one of these reasons:

| Reason | What to do |
| --- | --- |
| Not signed | Sign it: `codesign -s - --force --deep YourPlugin.clap` |
| Signed by an unknown developer — turn on Allow plugins I built myself to use it | Turn on **Allow plugins I built myself** in the Visualizer menu, or sign with a Developer ID |
| Quarantined (downloaded) — run: xattr -dr com.apple.quarantine <path> | Run the command shown, with the plugin's path, then restart AudioTap |
| No clap_entry | The file isn't a CLAP plugin, or `clap_entry` isn't exported. Mark it `CLAP_EXPORT`, and don't strip it |
| CLAP version not supported | Rebuild against CLAP 1.x headers (major version 1) |
| init failed | `clap_entry.init` returned false. Check what it needs; it receives the bundle's path |
| Not an audio effect | Add the `audio-effect` feature to the descriptor. Instruments and analyzers aren't offered |
| No mono or stereo audio ports | Give the plugin one main mono or stereo input and one main mono or stereo output (`audio-ports`) |
| Duplicate id | Another plugin has the same id. Change your plugin's `id` |
| Crashed while loading | The plugin crashed AudioTap and is skipped until its file changes. Fix the crash, rebuild, and replace the file |
| Stopped: <reason> | The plugin was bypassed for the session: non-finite output, using more than half the buffer period eight times running, or `CLAP_PROCESS_ERROR`. Fix the cause and restart AudioTap |

A plugin whose ports don't match the current sound is listed as "Needs mono" or "Needs stereo".

## Building for Linux

CLAP plugins in plain C or C++ have no macOS-specific code. The same source, built on Linux, becomes the plugin the
planned AudioFly server hosts.

- Build a shared object named `YourPlugin.clap` (no `lib` prefix, `.clap` instead of `.so`), with the `clap_entry`
  symbol visible (`CLAP_EXPORT`) and `-fvisibility=hidden` for everything else.
- Link `-lm` if you use the maths library.
- Keep macOS specifics out of the shared source: no Cocoa, CoreFoundation or other Apple frameworks. The example
  builds both from one `CMakeLists.txt` and one source file.
- Run `clap-validator` on the Linux build too (there is a Linux release).

Hosting a Linux plugin in the AudioFly server is not built yet; its requirements are separate.

## What AudioTap doesn't do

- It doesn't open the plugin's own window (`clap.gui`).
- It doesn't send note or MIDI events, and offers no sidechain input.
- It doesn't host VST3 or Audio Unit plugins.
- It doesn't run plugins outside its own process.

## Changes

- **Version 1** (2026-10): first version.
