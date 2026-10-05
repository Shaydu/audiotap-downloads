# AudioTap ISF host profile

**Profile version 1** · for ISF 2.0 · applies to AudioTap for macOS

This profile says exactly what AudioTap supports from the
[Interactive Shader Format (ISF) 2.0 specification](https://github.com/mrRay/ISF_Spec).
It doesn't restate the standard: read the ISF spec for the file format, then this page for what AudioTap supplies and
accepts. A shader that follows both loads in AudioTap.

## Where ISF visualizers run

| Host | ISF visualizers |
| --- | --- |
| **AudioTap** (macOS) | **Supported** (this profile) |
| AudioFly server | Not applicable: the server draws nothing. Planned: it stores shaders and delivers them to apps |
| AudioFly web app | Planned |
| AudioFly macOS, iPhone, iPad and Apple TV apps | Planned |

Planned hosts are not built yet. Follow the portability rules below and your shader is ready for them.

## What a visualizer is

- **One file**, ISF 2.0, extension `.fs`. Optional: a vertex shader with the same base name and the extension `.vs`,
  and the image files named in the header's `IMPORTED` section, all in the same folder as the `.fs`. `IMPORTED` paths
  are plain file names in that folder: no folders, no `..`, no absolute paths.
- **Limits:** the `.fs` file is at most 1 MB, with at most 16 `PASSES`, each at most 8192 × 8192 pixels. A pass's
  `WIDTH`/`HEIGHT` may be a number or an expression of up to 256 characters using numbers, `$WIDTH`, `$HEIGHT`, the
  `$NAME` of your own `float`, `long` or `bool` inputs, `+ - * /`, parentheses and `min max floor ceil round abs pow
  sqrt exp log mod sin cos tan clamp`. AudioTap checks it with a 3840 × 2160 output and each input at its `MAX`, so
  `$WIDTH*2` is fine and `$WIDTH*3` is too large. Each imported image is at most 32 MB and 8192 × 8192 pixels.
- **A generator.** It draws from audio, not from an input image. A shader with an `image` input named `inputImage`
  (an ISF filter) or an ISF transition is rejected.
- **Name:** the file name without `.fs`, with spaces added before capital letters (`NebulaPulse.fs` shows as
  "Nebula Pulse"). Use letters, digits, `-` and `_` in the file name.
- **Id:** `isf.` plus the file name without `.fs`, lower-cased. Two shaders with the same id: the first loaded wins,
  and the other is listed as "Duplicate name".
- **Detail line:** the header's `DESCRIPTION`, then `CREDIT` (shown as "by …"). Keep `DESCRIPTION` under 60
  characters.
- Set `"ISFVSN": "2"` in the header.

## Inputs AudioTap supplies

| ISF input `TYPE` | What AudioTap puts in it |
| --- | --- |
| `audioFFT` | The spectrum (see below) |
| `audio` | The waveform (see below) |
| `float`, `long`, `bool`, `color`, `point2D`, `event` | A control the listener can change (see Controls) |
| `image` | **Not supported**, except images listed in `IMPORTED`. A shader with another `image` input is rejected |

One `audioFFT` and one `audio` input is all you need; if you declare more, each gets the same data. Read them with `IMG_NORM_PIXEL` or `IMG_PIXEL`, as the ISF spec
says.

### `audioFFT`: the spectrum

- **Width:** 256 columns, or the input's `MAX` if it is smaller (1 to 256). With a smaller `MAX`, neighbouring bands
  are averaged.
- **Height:** 1 row (the spectrum is mono: left and right mixed).
- **Columns are log-spaced:** column 0 is 30 Hz, the last column is 16 kHz, with equal spacing in octaves. Bass is on
  the left, and each octave gets the same width. This differs from some other ISF hosts, which use linear FFT bins.
- **Values:** each band's level in dBFS, mapped from −96…0 dBFS to 0…1 (0 is −96 dBFS or quieter, 1 is full scale).
  Read the red channel (`.r`). Typical music sits between 0.4 and 0.85.
- Updated at the display's frame rate, from a 4096-point FFT with light smoothing, so it already moves smoothly.

### `audio`: the waveform

- **Width:** 1024 samples, or the input's `MAX` (1 to 8192).
- **Height:** 2 rows. Row 0 is the left channel, row 1 the right. With mono audio both rows are the same. Sample with
  `y = 0.25` for left and `y = 0.75` for right.
- **Order:** oldest sample on the left, newest on the right.
- **Values:** mapped from −1…1 to 0…1, so 0.5 is silence. Read `.r`, then use `x * 2.0 - 1.0` to get −1…1 back.
- At 48 kHz, 1024 samples are about 21 ms of sound.

### Timing

Audio inputs are delayed to match what the listener hears. That includes AirPlay, which can be two seconds behind,
so the picture stays in step with the sound. You don't need to do anything for this.

## Built-in values

`TIME`, `TIMEDELTA`, `FRAMEINDEX`, `DATE`, `RENDERSIZE`, `PASSINDEX` and `isf_FragNormCoord` behave as the ISF spec
says. `TIME` starts at 0 when the listener chooses your visualizer.

## Multiple passes

`PASSES` are supported, including `TARGET`, `PERSISTENT`, `FLOAT`, and `WIDTH`/`HEIGHT` expressions. A persistent
buffer starts cleared to transparent black. Use one for smoothing, trails or feedback.

## Controls

Every `float`, `long`, `bool`, `color`, `point2D` and `event` input gets a control in AudioTap's **Controls** panel,
in the order of your `INPUTS`:

| Input | Control | Uses from your header |
| --- | --- | --- |
| `float` | Slider | `MIN`, `MAX`, `DEFAULT` |
| `long` with `VALUES` and `LABELS` | Menu | `VALUES`, `LABELS`, `DEFAULT` |
| `long` without them | Slider in whole steps | `MIN`, `MAX`, `DEFAULT` |
| `bool` | Switch | `DEFAULT` |
| `color` | Colour well | `DEFAULT` (RGBA, 0…1) |
| `point2D` | 2-D pad | `MIN`, `MAX`, `DEFAULT` |
| `event` | Trigger button | true for one frame when pressed |

The control's label is your `LABEL`, or `NAME` if there's no `LABEL`. Always give `float` and `point2D` inputs `MIN`,
`MAX` and `DEFAULT`. Values are remembered per shader. Turn the qualities a listener would want to adjust into inputs
(colour, speed, sensitivity, smoothing). Keep the list short; four to six controls is plenty.

## The now-playing caption

AudioTap draws the track's title and artist over every visualizer, in its own style and timing. Don't draw text, and
leave the bottom-left corner uncluttered.

## Installing

Put the `.fs` file (and any `.vs` or images it uses) in:

```
~/Library/Application Support/AudioTap/Plugins/Visualizers/
```

In AudioTap, **Visualizer ▾ → Show Plugins Folder** opens it. You don't need to restart AudioTap:

- a new shader appears in the Visualizer menu within a couple of seconds;
- saving changes to a shader reloads it in place;
- deleting a shader removes it.

Shaders are not code that macOS runs, so they don't need signing, and the "Allow plugins I built myself" switch
doesn't apply to them.

## Checking a shader

With AudioTap installed, run:

```
/Applications/AudioTap.app/Contents/MacOS/AudioTap --check-plugin path/to/YourShader.fs
```

It reads the header, checks it against this profile, translates and compiles the shader, and draws a few frames from
a test signal. It prints `OK` and the inputs it found, or the reason it failed, with the line number in your `.fs` for
compile errors. Everything is printed to standard output. Exit status: `0` the shader is fine, `1` it failed (including
a file that doesn't exist, or a file that isn't a `.fs` shader: "Only ISF shaders (.fs files) can be checked: …"), `2` no
file was given. A shader that draws too slowly fails with "Stopped: took too long to draw. Save the file again to
retry." AudioTap doesn't need to be running, and the check plays no sound.

## When a shader doesn't load

The **Plugins folder** section of the **Visualizer ▾** menu lists every file in the plugins folder. Loaded files have a green dot; files that didn't load have a
red dot and one of these reasons:

| Reason | What to do |
| --- | --- |
| The header isn't valid ISF JSON: … | Fix the JSON in the `/* … */` header. The message names the problem |
| Unsupported input: … | Remove that input. See "Inputs AudioTap supplies" |
| This is a filter, not a visualizer | Draw from `audioFFT` or `audio` instead of `inputImage` |
| The shader didn't compile: line N, … | Fix line N of the `.fs`. Line numbers count from the top of the file, header included |
| Duplicate name: "…" is already used by another visualizer | Rename the file |
| Stopped: took too long to draw | Simplify the shader. Save the file again to retry |
| The shader couldn't start: … | Usually a Mac without a suitable GPU; the message gives the detail |
| The shader is too large (over 1 MB) | Shorten the file |
| Too many passes (the limit is 16) | Use 16 `PASSES` or fewer |
| A pass is too large (the limit is 8192 × 8192) | Lower that pass's `WIDTH`/`HEIGHT` |
| Imported images must be in the same folder as the shader | Put the images next to the `.fs` and name them without folders |
| An imported image is too large (the limit is 32 MB and 8192 × 8192) | Use a smaller image |
| The header isn't valid ISF JSON: an imported image couldn't be read | Each imported image must be an ordinary image file next to the `.fs` |
| The header isn't valid ISF JSON: the file couldn't be read | The `.fs` must be an ordinary readable file, not a folder, pipe or device |

AudioTap stops a shader that takes more than 50 ms of GPU time per frame for 30 frames in a row, or that makes the
GPU report an error. It then switches to its default visualizer.

## Portability rules

These keep a shader working in AudioTap today and in the planned AudioFly hosts, including the web app (WebGL):

- Write the GLSL the ISF spec uses: `gl_FragColor`, `IMG_NORM_PIXEL`/`IMG_PIXEL`/`IMG_SIZE`, no `#version` line.
- Don't use `texture2D` or `texture` on ISF images directly; use the `IMG_` functions.
- Loops need a constant upper bound (`for (int i = 0; i < 32; i++)`); `break` early if you need fewer.
- No integer bitwise operators, no `switch`, no arrays of arrays, no extensions, no derivatives (`dFdx`, `fwidth`).
- Don't depend on uninitialised values; initialise every variable.
- Keep it light: aim for under 4 ms per frame at 1920×1080 on an Apple M1.

## Changes

- **Version 1** (2026-10): first version.
