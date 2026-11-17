/* Block Shooter for GFOS: the rules (amiga/bs_core.h), as upstream's
 * main.js has them, with two of its slips put right: a block sliding off
 * the left or right edge goes back to that row's queue (upstream filed it
 * by column), and a block with a left arrow in the second column slides
 * like any other (upstream held it there).
 * MIT, like Block Shooter. Copyright (c) 2026 AmiCradle Team. */
#include "bs_core.h"

#define SQX(i) (BS_FIELD_X + ((i) % BS_N) * BS_CELL)
#define SQY(i) (BS_FIELD_Y + ((i) / BS_N) * BS_CELL)
#define FIELD_END (BS_FIELD_X + BS_N * BS_CELL)   /* 416 */

static unsigned rnd(bs_game *g, unsigned n)
{
	g->seed = (g->seed * 1103515245UL + 12345UL) & 0xffffffffUL;   /* the same on 32 and 64 bits */
	return (unsigned)((g->seed >> 16) & 0x7fff) % n;
}

static unsigned char side_block(bs_game *g)
{
	return (unsigned char)(1 + rnd(g, BS_COLOURS));
}

void bs_new_field(bs_game *g)
{
	/* upstream's middle 4 x 4: half the squares get a block, colours in its order */
	static const unsigned char pick[10] = { 2, 0, 4, 0, 3, 0, 1, 0, 5, 0 };
	int i;

	for (i = 0; i < BS_N * BS_N; i++)
		g->mf[i] = 0;
	for (i = 0; i < BS_N * BS_Q; i++) {
		g->tf[i] = side_block(g);
		g->rf[i] = side_block(g);
		g->bf[i] = side_block(g);
		g->lf[i] = side_block(g);
	}
	for (i = 0; i < 16; i++)
		g->mf[i / 4 + (i % 4) * 10 + 33] = pick[rnd(g, 10)];
	g->move.piece = 0;
}

void bs_new_game(bs_game *g, unsigned long seed)
{
	g->seed = seed;
	g->score = 0;
	bs_new_field(g);
	g->state = BS_PLAY;
}

static void fly(bs_game *g, unsigned char piece, int x0, int y0, int x1, int y1, int to)
{
	g->move.piece = piece;
	g->move.x0 = x0; g->move.y0 = y0;
	g->move.x1 = x1; g->move.y1 = y1;
	g->move.to = to;
	g->move.shot = 1;
	g->state = BS_MOVING;
}

int bs_click(bs_game *g, int x, int y)
{
	int i, line;

	if (g->state != BS_PLAY)
		return 0;
	if (x >= 0 && x <= 64 && y >= 0 && y <= 64) {
		g->state = BS_OVER;
		return 2;
	}
	if (x < 0 || y < 0 || x >= BS_SIZE || y >= BS_SIZE)
		return 0;
	if (x < BS_FIELD_X && y >= BS_FIELD_Y && y < FIELD_END) {
		line = (y - BS_FIELD_Y) / BS_CELL;
		for (i = 0; i < BS_N && !g->mf[line * BS_N + i]; i++)
			;
		if (i == BS_N || i == 0)
			return 0;
		fly(g, BS_PIECE(g->lf[line * 3 + 2], BS_RIGHT), BS_FIELD_X, SQY(line * BS_N),
			SQX(i - 1), SQY(line * BS_N), line * BS_N + i - 1);
		g->lf[line * 3 + 2] = g->lf[line * 3 + 1];
		g->lf[line * 3 + 1] = g->lf[line * 3];
		g->lf[line * 3] = side_block(g);
		return 1;
	}
	if (x >= FIELD_END && y >= BS_FIELD_Y && y < FIELD_END) {
		line = (y - BS_FIELD_Y) / BS_CELL;
		for (i = BS_N - 1; i >= 0 && !g->mf[line * BS_N + i]; i--)
			;
		if (i < 0 || i == BS_N - 1)
			return 0;
		fly(g, BS_PIECE(g->rf[line * 3], BS_LEFT), FIELD_END - BS_CELL, SQY(line * BS_N),
			SQX(i + 1), SQY(line * BS_N), line * BS_N + i + 1);
		g->rf[line * 3] = g->rf[line * 3 + 1];
		g->rf[line * 3 + 1] = g->rf[line * 3 + 2];
		g->rf[line * 3 + 2] = side_block(g);
		return 1;
	}
	if (y < BS_FIELD_Y && x >= BS_FIELD_X && x < FIELD_END) {
		line = (x - BS_FIELD_X) / BS_CELL;
		for (i = 0; i < BS_N && !g->mf[i * BS_N + line]; i++)
			;
		if (i == BS_N || i == 0)
			return 0;
		fly(g, BS_PIECE(g->tf[20 + line], BS_DOWN), SQX(line), BS_FIELD_Y,
			SQX(line), SQY((i - 1) * BS_N), (i - 1) * BS_N + line);
		g->tf[20 + line] = g->tf[10 + line];
		g->tf[10 + line] = g->tf[line];
		g->tf[line] = side_block(g);
		return 1;
	}
	if (y >= FIELD_END && x >= BS_FIELD_X && x < FIELD_END) {
		line = (x - BS_FIELD_X) / BS_CELL;
		for (i = BS_N - 1; i >= 0 && !g->mf[i * BS_N + line]; i--)
			;
		if (i < 0 || i == BS_N - 1)
			return 0;
		fly(g, BS_PIECE(g->bf[line], BS_UP), SQX(line), FIELD_END - BS_CELL,
			SQX(line), SQY((i + 1) * BS_N), (i + 1) * BS_N + line);
		g->bf[line] = g->bf[10 + line];
		g->bf[10 + line] = g->bf[20 + line];
		g->bf[20 + line] = side_block(g);
		return 1;
	}
	return 0;
}

/* a block off the field goes into the queue on that side, next to the
 * field, without its arrow; the queue's outermost block is dropped */
static void file_away(bs_game *g, int side, int line, unsigned char colour)
{
	switch (side) {
	case BS_SIDE_TOP:
		g->tf[line] = g->tf[10 + line];
		g->tf[10 + line] = g->tf[20 + line];
		g->tf[20 + line] = colour;
		break;
	case BS_SIDE_BOTTOM:
		g->bf[20 + line] = g->bf[10 + line];
		g->bf[10 + line] = g->bf[line];
		g->bf[line] = colour;
		break;
	case BS_SIDE_LEFT:
		g->lf[line * 3] = g->lf[line * 3 + 1];
		g->lf[line * 3 + 1] = g->lf[line * 3 + 2];
		g->lf[line * 3 + 2] = colour;
		break;
	default:
		g->rf[line * 3 + 2] = g->rf[line * 3 + 1];
		g->rf[line * 3 + 1] = g->rf[line * 3];
		g->rf[line * 3] = colour;
		break;
	}
}

void bs_land(bs_game *g)
{
	if (g->state != BS_MOVING)
		return;
	if (g->move.to >= 0) {
		g->mf[g->move.to] = g->move.piece;
		/* a shot block is matched first (upstream: sod 2); a slid one
		 * looks for more slides (sod 1) */
		g->state = g->move.shot ? BS_MATCH : BS_SLIDE;
	} else {
		int code = -1 - g->move.to;

		file_away(g, code / BS_N, code % BS_N, (unsigned char)BS_COLOUR(g->move.piece));
		g->state = BS_SLIDE;
	}
	g->move.piece = 0;
}

/* the next arrow block that can slide, scanning row by row as upstream
 * does: sets it moving and returns 1 */
static int slide_one(bs_game *g)
{
	int i, k, row, col, to, side = 0, line = 0, out;
	unsigned char p;

	for (i = 0; i < BS_N * BS_N; i++) {
		p = g->mf[i];
		if (!p || !BS_DIR(p))
			continue;
		row = i / BS_N;
		col = i % BS_N;
		k = 0;
		switch (BS_DIR(p)) {
		case BS_UP:
			while (row - k - 1 >= 0 && !g->mf[i - (k + 1) * BS_N]) k++;
			out = k == row; to = i - k * BS_N; side = BS_SIDE_TOP; line = col;
			break;
		case BS_DOWN:
			while (row + k + 1 < BS_N && !g->mf[i + (k + 1) * BS_N]) k++;
			out = k == BS_N - 1 - row; to = i + k * BS_N; side = BS_SIDE_BOTTOM; line = col;
			break;
		case BS_LEFT:
			while (col - k - 1 >= 0 && !g->mf[i - k - 1]) k++;
			out = k == col; to = i - k; side = BS_SIDE_LEFT; line = row;
			break;
		default:
			while (col + k + 1 < BS_N && !g->mf[i + k + 1]) k++;
			out = k == BS_N - 1 - col; to = i + k; side = BS_SIDE_RIGHT; line = row;
			break;
		}
		if (!k)
			continue;
		g->mf[i] = 0;
		g->move.piece = p;
		g->move.x0 = SQX(i); g->move.y0 = SQY(i);
		g->move.x1 = SQX(to); g->move.y1 = SQY(to);
		g->move.shot = 0;
		g->move.to = out ? -1 - (side * BS_N + line) : to;
		g->state = BS_MOVING;
		return 1;
	}
	return 0;
}

int bs_group(const bs_game *g, int i, int *out)
{
	unsigned char seen[BS_N * BS_N] = { 0 };
	int n = 0, head = 0, j, colour = BS_COLOUR(g->mf[i]);

	if (!g->mf[i])
		return 0;
	out[n++] = i;
	seen[i] = 1;
	while (head < n) {
		int c = out[head++];
		int next[4], m = 0;

		if (c >= BS_N) next[m++] = c - BS_N;
		if (c % BS_N > 0) next[m++] = c - 1;
		if (c < BS_N * (BS_N - 1)) next[m++] = c + BS_N;
		if (c % BS_N < BS_N - 1) next[m++] = c + 1;
		for (j = 0; j < m; j++)
			if (!seen[next[j]] && g->mf[next[j]] && BS_COLOUR(g->mf[next[j]]) == colour) {
				seen[next[j]] = 1;
				out[n++] = next[j];
			}
	}
	return n;
}

int bs_edge_full(const bs_game *g)
{
	int i;

	for (i = 0; i < BS_N * BS_N; i++)
		if ((i < BS_N || i >= BS_N * (BS_N - 1) || i % BS_N == 0 || i % BS_N == BS_N - 1) && !g->mf[i])
			return 0;
	return 1;
}

static int empty(const bs_game *g)
{
	int i;

	for (i = 0; i < BS_N * BS_N; i++)
		if (g->mf[i])
			return 0;
	return 1;
}

void bs_step(bs_game *g)
{
	int i, j, n, gone = 0, group[BS_N * BS_N];

	switch (g->state) {
	case BS_SLIDE:
		if (!slide_one(g))
			g->state = empty(g) ? BS_CLEARED : BS_MATCH;
		break;
	case BS_MATCH:
		for (i = 0; i < BS_N * BS_N; i++) {
			n = bs_group(g, i, group);
			if (n > 2) {
				gone = 1;
				g->score += (n - 2) * 3;
				for (j = 0; j < n; j++)
					g->mf[group[j]] = 0;
			}
		}
		g->state = gone ? BS_SLIDE : bs_edge_full(g) ? BS_OVER : BS_PLAY;
		break;
	case BS_CLEARED:
		bs_new_field(g);
		g->score += 100;
		g->state = BS_PLAY;
		break;
	default:
		break;
	}
}
