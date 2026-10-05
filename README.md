# AudioTap downloads

Alpha builds of AudioTap for macOS. Get the latest from **Releases** (the panel on the right).

## Install

1. Download the `.dmg` from the latest release and open it.
2. Drag **AudioTap** to **Applications**.
3. Open AudioTap. The first time, macOS asks to allow audio recording. Allow it. AudioTap needs it to hear what your Mac plays.

The app is signed with a Developer ID and notarized by Apple, so it opens normally.

## Requirements

macOS 14.2 or later, on an Apple silicon Mac (M1 or newer). Intel Macs are not supported in this alpha.

## Write a visualizer plugin

AudioTap visualizers are [ISF](https://github.com/mrRay/ISF_Spec) shaders: one `.fs` text file, no compiler or signing
needed. Drop it into `~/Library/Application Support/AudioTap/Plugins/Visualizers/` (or use **Visualizer ▾ → Show
Plugins Folder**) and it appears in the Visualizer menu within seconds; saving the file reloads it.

- The contract: [ISF host profile](plugins/audiotap-plugin-kit/skills/write-visualizer/isf-host-profile.md)
- A working example: [LevelBars.fs](plugins/audiotap-plugin-kit/skills/write-visualizer/examples/LevelBars.fs)

Let a coding agent write one for you. With Claude Code:

```
/plugin marketplace add Shaydu/audiotap-downloads
/plugin install audiotap-plugin-kit@audiotap
```

then describe the visualizer you want. Other agents: point them at [AGENTS.md](AGENTS.md). More in
[CONTRIBUTING.md](CONTRIBUTING.md).

## Send us feedback

In the app, use **Help > Report a Bug or Improvement**, or **Test checklist** to send results.

## Third-party notices

The licence notices for included third-party work are in the app, at
`AudioTap.app/Contents/Resources/THIRD_PARTY_NOTICES.txt`.
