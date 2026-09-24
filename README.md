# Chronomancer Noriol — AzerothCore Module

Adds **Chronomancer Noriol**, a mysterious NPC who manipulates timelines on your behalf.

## Features

- **Instance Lockout Reset** — Resets all raid and dungeon lockouts for you **and your own Playerbots** in your group.
  Other real players (and bots belonging to them) are never touched. The lockout of the map you are standing in is kept.
- **Playerbot Boost** — Instantly boost your own Playerbots to the max player level, one by one, with an optional
  gold cost per bot. Gear, talents and spells are not handled; use the Playerbots commands for that.
- **Outland Skip** — At level 58 (configurable), Noriol can boost your main character directly to level 68 to skip
  Outland entirely. Weapon and other level-based skills are raised to their new cap.
- **Lore-Friendly Dialogues** — Immersive NPC text and branching gossip options.
- **Configurable Costs** for each service:
  - Instance reset: default **10 gold**
  - Bot boost: default **1000 gold** per bot
  - Outland skip: default **5000 gold**
- **Spell FX and Emotes** — Time-altering animations to enhance immersion.

## Requirements

- [mod-playerbots](https://github.com/mod-playerbots/mod-playerbots) and its AzerothCore fork. This module includes
  Playerbots headers, so the build fails if mod-playerbots is missing or disabled.

## Installation

- Checkout the module folder to your AzerothCore modules folder.
- Run CMake and build AzerothCore.
- The SQL in `data/sql/db-world/` (creature template and NPC texts) is applied automatically by the worldserver
  database updater. If you disabled automatic module updates, import it into `acore_world` manually.
- Copy `mod-playerbot-reset-instances.conf.dist` to `mod-playerbot-reset-instances.conf` and adjust the settings.

## Usage

- The SQL only creates the NPC template; spawn Chronomancer Noriol wherever you like with **.npc add 190012**.

## Notes

- Instance Reset and Playerbot Boost are available at the max player level (80 by default).
- The Outland skip is available at `Chronomancer.SkipOutlandFromLevel` (58 by default).
- `Chronomancer.EnableGoldCost = 0` makes every service free.

## Upgrading

- The config file was renamed from `mod-playerbots-instance-reset.conf` to `mod-playerbot-reset-instances.conf`.
  Rename your existing file, or its settings are ignored and the defaults apply.
- The NPC texts now come from `npc_text` (IDs 190012 and 190013). If you imported the old SQL by hand, make sure the
  new SQL is applied, or the gossip window shows no text.

Enjoy your adventures in time!
