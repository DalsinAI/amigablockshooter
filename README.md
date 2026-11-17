# Block Shooter for GFOS

towerofglass's Block Shooter (a BrickShooter remake), rewritten in C for
AmigaOS 3.2 with GFOpenUp (68040/68060), on GFOpenUp's SDL 2. See
[AMIGA_PORT.md](AMIGA_PORT.md) for what the port is and
[amiga/Licences.txt](amiga/Licences.txt) for the licences.

Upstream is not kept here: `package/build-amiga.sh` fetches it at the
commit in `package/blockshooter.lock`, for its sprite sheet.

- `amiga/`: the rules (`bs_core.c`), the picture and mouse on SDL 2
  (`bs_main.c`), the icons, Controls and the Installer spec.
- `package/`: the build and package scripts (Cradle stove_build, target `package`).
- `tests/host/`: host tests (`sh tests/host/test_all.sh`).
- `tools/`: the PNG reader for the sprite sheet, and the LhA packer.

MIT, Copyright (c) 2026 AmiCradle Team.
Designed, tested and delivered by The AmiCradle Team.
