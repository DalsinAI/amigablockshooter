/* Block Shooter for GFOS: the picture, the mouse and the clock, on
 * GFOpenUp's SDL 2 (SDL2.module). The rules are amiga/bs_core.c; the blocks
 * are upstream's sprite sheet (blocks-arrows.3.png, built in by
 * tools/png2c.py). Upstream draws its words with the browser's monospace
 * font; here they are a small block font of our own.
 * MIT, like Block Shooter. Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <SDL.h>
#include "bs_core.h"

extern const unsigned int bs_sheet_w, bs_sheet_h;
extern const unsigned long bs_sheet[];

#define MOVE_MS 100               /* upstream's tween time */

/* 5 x 7 letters, a row a byte, the high bit of 5 on the left */
static const struct { char c; unsigned char rows[7]; } glyphs[] = {
	{ '0', { 14, 17, 19, 21, 25, 17, 14 } }, { '1', { 4, 12, 4, 4, 4, 4, 14 } },
	{ '2', { 14, 17, 1, 2, 4, 8, 31 } }, { '3', { 31, 2, 4, 2, 1, 17, 14 } },
	{ '4', { 2, 6, 10, 18, 31, 2, 2 } }, { '5', { 31, 16, 30, 1, 1, 17, 14 } },
	{ '6', { 6, 8, 16, 30, 17, 17, 14 } }, { '7', { 31, 1, 2, 4, 8, 8, 8 } },
	{ '8', { 14, 17, 17, 14, 17, 17, 14 } }, { '9', { 14, 17, 17, 15, 1, 2, 12 } },
	{ 'A', { 14, 17, 17, 31, 17, 17, 17 } }, { 'C', { 14, 17, 16, 16, 16, 17, 14 } },
	{ 'E', { 31, 16, 16, 30, 16, 16, 31 } }, { 'G', { 14, 17, 16, 23, 17, 17, 15 } },
	{ 'I', { 14, 4, 4, 4, 4, 4, 14 } }, { 'K', { 17, 18, 20, 24, 20, 18, 17 } },
	{ 'L', { 16, 16, 16, 16, 16, 16, 31 } }, { 'M', { 17, 27, 21, 21, 17, 17, 17 } },
	{ 'N', { 17, 17, 25, 21, 19, 17, 17 } }, { 'O', { 14, 17, 17, 17, 17, 17, 14 } },
	{ 'P', { 30, 17, 17, 30, 16, 16, 16 } }, { 'R', { 30, 17, 17, 30, 20, 18, 17 } },
	{ 'S', { 15, 16, 16, 14, 1, 1, 30 } }, { 'T', { 31, 4, 4, 4, 4, 4, 4 } },
	{ 'U', { 17, 17, 17, 17, 17, 17, 14 } }, { 'V', { 17, 17, 17, 17, 17, 10, 4 } },
	{ 'Y', { 17, 17, 10, 4, 4, 4, 4 } },
};

static void text(SDL_Renderer *r, int x, int y, int scale, const char *s)
{
	size_t g;
	int row, col;

	for (; *s; s++, x += 6 * scale) {
		for (g = 0; g < sizeof glyphs / sizeof glyphs[0] && glyphs[g].c != *s; g++)
			;
		if (g == sizeof glyphs / sizeof glyphs[0])
			continue;
		for (row = 0; row < 7; row++)
			for (col = 0; col < 5; col++)
				if (glyphs[g].rows[row] & (16 >> col)) {
					SDL_Rect px = { x + col * scale, y + row * scale, scale, scale };
					SDL_RenderFillRect(r, &px);
				}
	}
}

/* a square of the sheet: colour 1..5 is the column, the arrow the row;
 * column 5, row 0 is the empty square */
static void block(SDL_Renderer *r, SDL_Texture *sheet, unsigned char piece, int x, int y)
{
	SDL_Rect from = { 160, 0, BS_CELL, BS_CELL }, to = { x, y, BS_CELL, BS_CELL };

	if (piece) {
		from.x = (BS_COLOUR(piece) - 1) * BS_CELL;
		from.y = BS_DIR(piece) * BS_CELL;
	}
	SDL_RenderCopy(r, sheet, &from, &to);
}

static void draw(SDL_Renderer *r, SDL_Texture *sheet, const bs_game *g, Uint32 moving_since)
{
	char num[24];
	int i;

	SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
	SDL_RenderClear(r);
	for (i = 0; i < BS_N * BS_N; i++)
		block(r, sheet, g->mf[i], BS_FIELD_X + (i % BS_N) * BS_CELL, BS_FIELD_Y + (i / BS_N) * BS_CELL);
	for (i = 0; i < BS_N * BS_Q; i++) {
		block(r, sheet, g->tf[i], BS_FIELD_X + (i % 10) * BS_CELL, (i / 10) * BS_CELL);
		block(r, sheet, g->bf[i], BS_FIELD_X + (i % 10) * BS_CELL, 416 + (i / 10) * BS_CELL);
		block(r, sheet, g->lf[i], (i % 3) * BS_CELL, BS_FIELD_Y + (i / 3) * BS_CELL);
		block(r, sheet, g->rf[i], 416 + (i % 3) * BS_CELL, BS_FIELD_Y + (i / 3) * BS_CELL);
	}
	if (g->state == BS_MOVING) {
		Uint32 t = SDL_GetTicks() - moving_since;
		int x, y;

		if (t > MOVE_MS)
			t = MOVE_MS;
		x = g->move.x0 + (int)((g->move.x1 - g->move.x0) * (long)t / MOVE_MS);
		y = g->move.y0 + (int)((g->move.y1 - g->move.y0) * (long)t / MOVE_MS);
		block(r, sheet, g->move.piece, x, y);
	}
	SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
	text(r, 426, 6, 2, "SCORE");
	snprintf(num, sizeof num, "%ld", g->score);
	text(r, 426, 36, 2, num);
	text(r, 6, 36, 2, "RESTART");
	if (g->state == BS_OVER) {
		SDL_Rect box = { 96, 200, 320, 112 };

		SDL_SetRenderDrawColor(r, 20, 20, 60, 255);
		SDL_RenderFillRect(r, &box);
		SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
		SDL_RenderDrawRect(r, &box);
		text(r, 148, 214, 4, "GAME OVER");
		snprintf(num, sizeof num, "SCORE %ld", g->score);
		text(r, 256 - (int)strlen(num) * 9, 258, 3, num);
		text(r, 256 - 19 * 6, 290, 2, "CLICK TO PLAY AGAIN");
	}
	SDL_RenderPresent(r);
}

int main(int argc, char **argv)
{
	SDL_Window *win;
	SDL_Renderer *r;
	SDL_Texture *sheet;
	SDL_Event ev;
	static Uint32 pixels[192 * 160];
	bs_game g;
	Uint32 moving_since = 0;
	int quit = 0, i, was_moving = 0;

	(void)argc;
	(void)argv;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Block Shooter: no SDL video: %s\n", SDL_GetError());
		return 20;
	}
	win = SDL_CreateWindow("Block Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		BS_SIZE, BS_SIZE, 0);
	r = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : NULL;
	if (win && !r)
		r = SDL_CreateRenderer(win, -1, 0);
	if (!r) {
		fprintf(stderr, "Block Shooter: no window: %s\n", SDL_GetError());
		SDL_Quit();
		return 20;
	}
	SDL_RenderSetLogicalSize(r, BS_SIZE, BS_SIZE);
	/* RGBA8888 is a packed format: 0xRRGGBBAA words on either byte order */
	for (i = 0; i < (int)(bs_sheet_w * bs_sheet_h) && i < 192 * 160; i++)
		pixels[i] = (Uint32)bs_sheet[i];
	sheet = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC,
		(int)bs_sheet_w, (int)bs_sheet_h);
	if (!sheet || SDL_UpdateTexture(sheet, NULL, pixels, (int)bs_sheet_w * 4) < 0) {
		fprintf(stderr, "Block Shooter: no texture: %s\n", SDL_GetError());
		SDL_Quit();
		return 20;
	}
	SDL_SetTextureBlendMode(sheet, SDL_BLENDMODE_BLEND);

	bs_new_game(&g, (unsigned long)time(NULL) ^ SDL_GetTicks());
	while (!quit) {
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				quit = 1;
			else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
				quit = 1;
			else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
				if (g.state == BS_OVER)
					bs_new_game(&g, g.seed);
				else
					bs_click(&g, ev.button.x, ev.button.y);
			}
		}
		if (g.state == BS_MOVING) {
			if (!was_moving)
				moving_since = SDL_GetTicks();
			was_moving = 1;
			if (SDL_GetTicks() - moving_since >= MOVE_MS) {
				bs_land(&g);
				was_moving = 0;
			}
		} else if (g.state == BS_SLIDE || g.state == BS_MATCH || g.state == BS_CLEARED)
			bs_step(&g);
		draw(r, sheet, &g, moving_since);
		if (g.state == BS_PLAY || g.state == BS_OVER)
			SDL_Delay(10);
	}
	SDL_DestroyTexture(sheet);
	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
