# Contributing

## Writing a plugin

You can write AudioTap plugins without access to AudioTap's source code:

- **Visualizers** are [ISF 2.0](https://github.com/mrRay/ISF_Spec) shaders. Start with the
  [ISF host profile](plugins/audiotap-plugin-kit/skills/write-visualizer/isf-host-profile.md) and the
  [LevelBars example](plugins/audiotap-plugin-kit/skills/write-visualizer/examples/LevelBars.fs).
- **Audio effects** will be [CLAP](https://github.com/free-audio/clap) plugins. Their host profile is coming soon.

The easiest way is to describe what you want to a coding agent. With Claude Code:

```
/plugin marketplace add Shaydu/audiotap-downloads
/plugin install audiotap-plugin-kit@audiotap
```

Other agents: point them at [AGENTS.md](AGENTS.md).

## Sharing your plugin

Your plugins are yours. Keep them in your own repository and share them however you like. Anyone can install a
shader by dropping the `.fs` file into AudioTap's plugins folder.

There is no plugin gallery yet.

## Improving the profiles and skills

If a profile is wrong or unclear, or an agent following a skill got stuck, open an issue or a pull request against
this repository. Say what you (or your agent) tried, what happened, and which part of the document misled you. Pull
requests here are for the profiles, skills, examples and these instructions, not for individual plugins.

## Licence

Everything in this repository except the AudioTap app downloads is under the [MIT licence](LICENSE). By sending a
pull request you agree that your change is under the same licence.
