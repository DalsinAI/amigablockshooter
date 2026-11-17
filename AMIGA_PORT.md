Block Shooter for GFOS
======================

Block Shooter, towerofglass's remake of the old puzzle game BrickShooter:
coloured blocks wait in rows round a 10 x 10 field. Shoot them in; three
or more of one colour touching vanish. For AmigaOS 3.2 with GFOpenUp, on a
68040 or 68060.

Running it
----------

- Double-click BlockShooter. It opens a 512 x 512 window through GFOpenUp's
  SDL 2 (LIBS:GFOpenGPU/SDL2.module).
- Controls and the rules: see Controls.

What the port is
----------------

Upstream is a web page: 295 lines of JavaScript on the Phaser engine. The
port is a rewrite in C:
- amiga/bs_core.c: the rules, as main.js has them, with no SDL in them, so
  the host tests check every one. Two slips are put right: a block sliding
  off the left or right edge goes back to the queue of its own row
  (main.js filed it by column), and a block with a left arrow in the
  second column slides like any other (main.js held it there).
- amiga/bs_main.c: the picture on SDL 2's renderer, 100 ms slides as
  upstream's tweens, the mouse; the words in a small block font of our own
  (upstream used the browser's monospace font). Game over is a box on the
  field, a click starts again (upstream used the browser's alert).
- tools/png2c.py: upstream's sprite sheet built into the program, decoded
  at build time with our own PNG reader.

Building
--------

package/build-amiga.sh on a Cradle stove (os32-gcc16-open6) fetches
upstream at the commit package/blockshooter.lock names, builds the game,
and writes the LhA archive and the GFOpenUp part. The host tests are in
tests/host (sh tests/host/test_all.sh).

Designed, tested and delivered by The AmiCradle Team.
