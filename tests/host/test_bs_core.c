/* Host test of amiga/bs_core.c, Block Shooter's rules: a new field, shots
 * from each side, sliding arrows, blocks filed back into a queue, matches
 * and their score, the cleared-field bonus, the Restart corner and the
 * game's end. Prints the same on the PC and on 68k.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include "bs_core.h"

static int fails;

static void check(int ok, const char *what)
{
	printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
	if (!ok)
		fails++;
}

/* runs the rules until they wait for a click, landing every move */
static void settle(bs_game *g)
{
	int n = 0;

	while (g->state != BS_PLAY && g->state != BS_OVER && n++ < 1000) {
		if (g->state == BS_MOVING)
			bs_land(g);
		else
			bs_step(g);
	}
}

static void clear(bs_game *g)
{
	memset(g->mf, 0, sizeof g->mf);
	g->state = BS_PLAY;
	g->score = 0;
}

int main(void)
{
	bs_game g;
	int i, n, outside = 0, group[100];
	unsigned long sum = 0;

	bs_new_game(&g, 1);
	for (i = 0; i < 100; i++) {
		int r = i / 10, c = i % 10;
		if (g.mf[i] && (r < 3 || r > 6 || c < 3 || c > 6 || BS_DIR(g.mf[i])))
			outside++;
		sum = (sum * 31 + g.mf[i]) & 0xffffffffUL;
	}
	for (i = 0; i < 30; i++)
		if (g.lf[i] < 1 || g.lf[i] > 5 || g.rf[i] < 1 || g.rf[i] > 5 || g.tf[i] < 1 || g.tf[i] > 5 || g.bf[i] < 1 || g.bf[i] > 5)
			outside++;
	check(!outside && g.state == BS_PLAY && g.score == 0, "new game: blocks only in the middle 4 x 4, queues full of colours 1-5");
	printf("field hash %lu\n", sum);

	/* a shot from the left stops next to the first block in its row */
	clear(&g);
	g.mf[5 * 10 + 6] = 1;
	g.lf[5 * 3 + 2] = 2; g.lf[5 * 3 + 1] = 3; g.lf[5 * 3] = 4;
	check(bs_click(&g, 50, 96 + 5 * 32 + 10) == 1 && g.state == BS_MOVING, "left queue click shoots");
	check(g.move.to == 55 && g.move.piece == BS_PIECE(2, BS_RIGHT) && g.move.x0 == 96 && g.move.x1 == 96 + 5 * 32,
		"shot lands at column 5: the queue's front block, arrow to the right");
	check(g.lf[17] == 3 && g.lf[16] == 4 && g.lf[15] >= 1 && g.lf[15] <= 5, "left queue moves up one, a new block at the back");
	settle(&g);
	check(g.mf[55] == BS_PIECE(2, BS_RIGHT) && g.state == BS_PLAY, "lands and waits");

	/* an empty row, or a block already at the edge: no shot */
	clear(&g);
	check(bs_click(&g, 50, 200) == 0 && g.state == BS_PLAY, "empty row: nothing to stop against");
	g.mf[30] = 1;
	check(bs_click(&g, 50, 96 + 3 * 32) == 0, "block on the edge: no room");

	/* shots from the right, top and bottom */
	clear(&g);
	g.mf[44] = 1;
	bs_click(&g, 450, 96 + 4 * 32 + 1);
	check(g.move.to == 45 && BS_DIR(g.move.piece) == BS_LEFT, "right queue shoots left");
	bs_land(&g); settle(&g);
	bs_click(&g, 96 + 4 * 32 + 5, 20);
	check(g.move.to == 34 && BS_DIR(g.move.piece) == BS_DOWN, "top queue shoots down");
	bs_land(&g); settle(&g);
	bs_click(&g, 96 + 4 * 32 + 5, 480);
	check(g.move.to == 54 && BS_DIR(g.move.piece) == BS_UP, "bottom queue shoots up");

	/* three of a colour vanish: (3 - 2) * 3 points */
	clear(&g);
	g.mf[52] = 3; g.mf[53] = 3; g.mf[54] = 2; g.mf[60] = 1;
	g.lf[5 * 3 + 2] = 3;
	bs_click(&g, 10, 96 + 5 * 32 + 1);
	settle(&g);
	check(!g.mf[51] && !g.mf[52] && !g.mf[53] && g.mf[54] == 2 && g.score == 3, "shot makes three in a row: they vanish, 3 points");
	n = bs_group(&g, 60, group);
	check(n == 1, "a lone block is a group of one");

	/* an L of five: (5 - 2) * 3 = 9 */
	clear(&g);
	g.mf[0] = 4; g.mf[10] = 4; g.mf[20] = 4; g.mf[21] = 4; g.mf[99] = 1;
	check(bs_group(&g, 0, group) == 4, "flood fill joins the L");
	g.mf[22] = 4;
	g.state = BS_MATCH;
	settle(&g);
	check(g.score == 9 && !g.mf[0] && g.mf[99] == 1, "five joined: 9 points");

	/* an arrow block slides when its way opens, and stops at the next block */
	clear(&g);
	g.mf[57] = BS_PIECE(1, BS_LEFT); g.mf[52] = 2;
	g.state = BS_SLIDE;
	bs_step(&g);
	check(g.state == BS_MOVING && g.move.to == 53 && !g.move.shot, "left arrow slides to the block");
	settle(&g);
	check(g.mf[53] == BS_PIECE(1, BS_LEFT) && !g.mf[57], "and stays there");

	/* one in the second column slides too (upstream held it) */
	clear(&g);
	g.mf[21] = BS_PIECE(1, BS_LEFT); g.mf[99] = 2;
	g.lf[2 * 3 + 2] = 5;
	g.state = BS_SLIDE;
	bs_step(&g);
	check(g.state == BS_MOVING && g.move.to == -1 - (BS_SIDE_LEFT * 10 + 2), "second-column left arrow slides off");
	bs_land(&g);
	check(g.lf[2 * 3 + 2] == 1 && g.lf[2 * 3 + 1] == 5 && !g.mf[21], "filed into its own row's queue, next to the field, no arrow");

	/* sliding off the top, bottom and right */
	clear(&g);
	g.mf[45] = BS_PIECE(3, BS_UP); g.mf[99] = 2;
	g.state = BS_SLIDE; bs_step(&g); bs_land(&g);
	check(g.tf[25] == 3, "off the top: into column 5's top queue");
	clear(&g);
	g.mf[45] = BS_PIECE(4, BS_DOWN); g.mf[0] = 2;
	g.state = BS_SLIDE; bs_step(&g); bs_land(&g);
	check(g.bf[5] == 4, "off the bottom: into column 5's bottom queue");
	clear(&g);
	g.mf[45] = BS_PIECE(5, BS_RIGHT); g.mf[0] = 2;
	g.state = BS_SLIDE; bs_step(&g); bs_land(&g);
	check(g.rf[4 * 3] == 5, "off the right: into row 4's right queue");

	/* the field emptied: 100 points and a new one */
	clear(&g);
	g.mf[0] = 1; g.mf[1] = 1; g.mf[2] = 1;
	g.state = BS_MATCH;
	settle(&g);
	check(g.score == 103 && g.state == BS_PLAY, "cleared field: 3 + 100 points, play on");
	for (i = n = 0; i < 100; i++)
		n += g.mf[i] != 0;
	check(n <= 16, "and a new middle");

	/* the edge full, no match: game over */
	clear(&g);
	for (i = 0; i < 100; i++)
		if (i < 10 || i >= 90 || i % 10 == 0 || i % 10 == 9)
			g.mf[i] = (unsigned char)(1 + (i % 2) + ((i / 10) % 2) * 2);
	check(bs_edge_full(&g), "edge full");
	g.state = BS_MATCH;
	settle(&g);
	check(g.state == BS_OVER, "edge full and nothing to match: game over");

	/* the Restart corner */
	bs_new_game(&g, 7);
	check(bs_click(&g, 30, 30) == 2 && g.state == BS_OVER, "Restart corner ends the game");
	check(bs_click(&g, 50, 200) == 0, "no shots once it is over");

	if (fails)
		printf("FAILED %d\n", fails);
	else
		printf("rules: all checks pass\n");
	return fails != 0;
}
