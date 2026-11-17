/* Block Shooter for GFOS: the game's rules, in C with no SDL, so the host
 * tests reach every one of them. A rewrite of towerofglass's Block Shooter
 * (main.js, Phaser): a 10 x 10 field with 3 rows of waiting blocks on each
 * side. Click a waiting block next to the field to shoot it in; it slides
 * until it meets a block. Three or more of one colour touching vanish.
 * Arrows on blocks keep them moving that way when the way opens; a block
 * that slides off the field goes back to the queue on that side. Clear the
 * field for 100 points and a new one. The game ends when every square on
 * the field's edge is taken.
 * MIT, like Block Shooter. Copyright (c) 2026 AmiCradle Team. */
#ifndef BS_CORE_H
#define BS_CORE_H

#define BS_N 10                   /* the field is BS_N x BS_N */
#define BS_Q 3                    /* waiting blocks per side line */
#define BS_CELL 32                /* pixels per square */
#define BS_COLOURS 5              /* red, yellow, green, blue, purple */

/* a square: 0 is empty; else colour 1..5 and a direction (bs_dir) */
#define BS_COLOUR(c) ((c) & 15)
#define BS_DIR(c) ((c) >> 4)
#define BS_PIECE(colour, dir) ((unsigned char)((colour) | ((dir) << 4)))
enum bs_dir { BS_STILL, BS_UP, BS_RIGHT, BS_DOWN, BS_LEFT };

enum bs_side { BS_SIDE_LEFT, BS_SIDE_RIGHT, BS_SIDE_TOP, BS_SIDE_BOTTOM };

enum bs_state {
	BS_PLAY,                      /* waiting for a click */
	BS_SLIDE,                     /* arrows to follow (upstream's "checkfield") */
	BS_MATCH,                     /* groups to take away ("seekndstry") */
	BS_MOVING,                    /* a block on its way: bs_move, then bs_land */
	BS_CLEARED,                   /* the field is empty: 100 points, a new one */
	BS_OVER                       /* the edge is full */
};

/* a block in flight, in the 512 x 512 picture's pixels */
typedef struct {
	unsigned char piece;
	int x0, y0, x1, y1;
	int shot;                     /* shot in from a queue (else slid by its arrow) */
	int to;                       /* 0..99 a field square; -1 - (side * BS_N + line) off the field */
} bs_move;

typedef struct {
	unsigned char mf[BS_N * BS_N];        /* the field, row by row */
	/* the queues, laid out as upstream draws them:
	 * lf[row * 3 + k], k = 2 next to the field; rf[row * 3 + k], k = 0 next
	 * to it; tf[k * 10 + col], k = 2 next to it; bf[k * 10 + col], k = 0 */
	unsigned char lf[BS_N * BS_Q], rf[BS_N * BS_Q], tf[BS_N * BS_Q], bf[BS_N * BS_Q];
	long score;
	int state;
	bs_move move;
	unsigned long seed;
} bs_game;

/* the picture: field at 96..416, side queues around it */
#define BS_FIELD_X 96
#define BS_FIELD_Y 96
#define BS_SIZE 512

void bs_new_game(bs_game *g, unsigned long seed);
void bs_new_field(bs_game *g);
/* a click at picture x, y while BS_PLAY: 1 if it shot a block (state is
 * then BS_MOVING), 2 for the Restart corner (state BS_OVER), else 0 */
int bs_click(bs_game *g, int x, int y);
/* the block in flight has arrived */
void bs_land(bs_game *g);
/* one step of the rules for BS_SLIDE, BS_MATCH, BS_CLEARED */
void bs_step(bs_game *g);
/* the squares joined to square i in its colour (upstream's recsearch);
 * out holds up to BS_N * BS_N; returns how many */
int bs_group(const bs_game *g, int i, int *out);
/* the 36 edge squares all taken */
int bs_edge_full(const bs_game *g);

#endif
