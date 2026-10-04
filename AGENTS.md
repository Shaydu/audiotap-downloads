# Building AudioTap plugins — instructions for coding agents

If a person asks you to make a visualizer or an audio effect for **AudioTap** (or AudioFly), follow this file. You
don't need AudioTap's source code; everything you need is in this repository.

AudioTap takes two kinds of plugin, each an existing open standard:

| The person wants | You make | Instructions | Contract |
| --- | --- | --- | --- |
| A visualizer: a picture that moves with the music | An **ISF 2.0 shader** (`.fs` file) | [write-visualizer](plugins/audiotap-plugin-kit/skills/write-visualizer/SKILL.md) | [ISF host profile](plugins/audiotap-plugin-kit/skills/write-visualizer/isf-host-profile.md) |
| An audio effect: something that changes the sound | A **CLAP** audio effect (`.clap`) | Coming soon | Coming soon |

## How to work

1. Open the instructions file for the kind of plugin and follow its steps in order. They are written as skills for
   Claude Code, but they are plain Markdown and apply to any agent.
2. Treat the host profile as the contract. Where the general standard (ISF or CLAP) allows something the profile
   doesn't, the profile wins.
3. Start from the example next to the instructions; it is known to load in AudioTap.
4. Check your plugin with the tool the instructions name before you install it, and fix everything it reports.
5. Install the plugin into the folder the profile names. AudioTap notices new plugins while it runs.
6. Finish with the short checklist the instructions describe, so the person knows where to find the plugin and what
   each control does.

## Rules

- Don't guess at behaviour the profile doesn't describe. If the profile is silent or unclear, say so in your answer;
  that's a gap in our documentation, and we want to hear about it (see CONTRIBUTING.md).
- Never overwrite a plugin file that isn't the one you're writing.
- Plugins you write belong to the person you wrote them for. The examples here are MIT-licensed, so copying from
  them is fine.

## Using Claude Code

```
/plugin marketplace add Shaydu/audiotap-downloads
/plugin install audiotap-plugin-kit@audiotap
```

Then describe the plugin; the matching skill takes over.
