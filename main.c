#include "game.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;

struct images images;

int board[BOARD_WIDTH][BOARD_HEIGHT];
float board_fall_times[BOARD_WIDTH][BOARD_HEIGHT];
float board_deltas[BOARD_WIDTH][BOARD_HEIGHT];

int pieces_moving;

int moving_col;
float vertical_rotation;
int moving_row;
float horizontal_delta;

int new_row[BOARD_WIDTH];
float new_row_delta;

int cursor_x = 0;
int cursor_y = 0;

int score;

/* TODO: hold time should only apply to matched pieces */
#define HOLD_TIME 0
#define FALL_SPEED (16*18)

static void add_new_row()
{
	int i, j;

	for (i = 0; i < BOARD_WIDTH; i++) {
		for (j = 0; j < BOARD_HEIGHT - 1; j++) {
			board[i][j] = board[i][j + 1];
		}
		board[i][BOARD_HEIGHT - 1] = new_row[i];
		new_row[i] = rand() % 2 ? WHITE : BLACK;
	}

	new_row_delta = 0;
}

int init(SDL_Surface **screen)
{
	assert(screen);

	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		fprintf(stderr, "SDL_Init failed.\n");
		return 1;
	}

	*screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT,
				  SCREEN_BPP,
				  SDL_SWSURFACE);

	if (*screen == NULL) {
		fprintf(stderr, "SDL_SetVideoMode failed.\n");
		return 1;
	}

	SDL_WM_SetCaption("Puzzle Game", NULL);

	return 0;
}

void load_files()
{
	images.black_image = load_image("assets/black.png");
	images.white_image = load_image("assets/white.png");

	images.black_to_white[0] = load_image("assets/bw1.png");
	images.black_to_white[1] = load_image("assets/bw2.png");
	images.black_to_white[2] = load_image("assets/bw3.png");
	images.black_to_white[3] = load_image("assets/bw4.png");

	images.black_scale[0] = load_image("assets/black_particle1.png");
	images.black_scale[1] = load_image("assets/black_particle2.png");
	images.black_scale[2] = load_image("assets/black_particle3.png");
	images.white_scale[0] = load_image("assets/white_particle1.png");
	images.white_scale[1] = load_image("assets/white_particle2.png");
	images.white_scale[2] = load_image("assets/white_particle3.png");

	images.background = load_image("assets/space.png");

	images.font = load_image("assets/font.png");
}

void clean_up()
{
	SDL_FreeSurface(images.white_image);
	SDL_FreeSurface(images.black_image);
	SDL_FreeSurface(images.black_to_white[0]);
	SDL_FreeSurface(images.black_to_white[1]);
	SDL_FreeSurface(images.black_to_white[2]);
	SDL_FreeSurface(images.black_to_white[3]);
	SDL_FreeSurface(images.black_scale[0]);
	SDL_FreeSurface(images.black_scale[1]);
	SDL_FreeSurface(images.black_scale[2]);
	SDL_FreeSurface(images.white_scale[0]);
	SDL_FreeSurface(images.white_scale[1]);
	SDL_FreeSurface(images.white_scale[2]);
	SDL_FreeSurface(images.background);
	SDL_FreeSurface(images.font);

	SDL_Quit();
}

static void draw_new_piece(int i, float dy, SDL_Surface *screen,
			   SDL_Rect *clip)
{
	switch (new_row[i] & 0x0F) {
	case BLACK:
		apply_surface(i * 32, 480 + dy,
			      images.black_image, screen,
			      clip);
		break;
	case WHITE:
		apply_surface(i * 32, 480 + dy,
			      images.white_image, screen,
			      clip);
		break;
	default:
		break;
	}
}

static void draw_piece(int i, int j, float dx, float dy,
		       SDL_Surface *screen, SDL_Rect *clip)
{
	int rot = -1;
	SDL_Surface *image = NULL;
	int k;

	if (moving_col == i)
		rot = (int) vertical_rotation;

	switch (board[i][j] & 0x0F) {
	case BLACK:
		if (rot > 0) {
			k = 3 - rot / 30;
			if (k < 0)
				k = 0;
			image = images.black_to_white[k];
		} else {
			image = images.black_image;
		}
		break;
	case WHITE:
		if (rot > 0) {
			k = rot / 30;
			if (k > 3)
				k = 3;
			image = images.black_to_white[k];
		} else {
			image = images.white_image;
		}
		break;
	default:
		break;
	}

	if (image)
		apply_surface(i * 32 + dx, j * 32 + dy,
			      image, screen, clip);
}

static void draw_board(SDL_Surface *screen)
{
	int i, j;
	float dx, dy;
	SDL_Rect clip;

	clip.x = 0;
	clip.y = 0;
	clip.w = 32;
	clip.h = 32;

	for (j = 0; j < BOARD_HEIGHT; j++) {
		for (i = 0; i < BOARD_WIDTH; i++) {

			if (j == moving_row)
				dx = horizontal_delta;
			else
				dx = 0;

			dy = -new_row_delta;
			dy += board_deltas[i][j];

			if (dx < 0 && i == 0) {
				clip.w = 32;
				draw_piece(i, j, dx, dy, screen, &clip);

				clip.w = -dx;
				dx += 10 * 32;
				draw_piece(i, j, dx, dy, screen, &clip);
			} else {
				clip.w = 32;
				draw_piece(i, j, dx, dy, screen, &clip);
			}
		}
	}

	clip.w = 32;
	clip.h = -new_row_delta;
	dy = -new_row_delta;

	for (i = 0; i < BOARD_WIDTH; i++) {
		draw_new_piece(i, dy, screen, &clip);
	}
}

void draw_score(SDL_Surface *screen)
{
	int s = score;
	int c;
	SDL_Rect clip;
	int x = 500;

	clip.y = 0;
	clip.w = 32;
	clip.h = 32;

	if (s == 0) {
		clip.x = 0;
		apply_surface(x, 100, images.font, screen, &clip);
		return;
	}

	while (s > 0) {
		c = s % 10;
		s /= 10;
		clip.x = c * 32;
		apply_surface(x, 100, images.font, screen, &clip);
		x -= 32;
	}
}

int draw(SDL_Surface *screen)
{
	apply_surface(0, 0, images.background, screen, NULL);

	draw_board(screen);
	draw_particles(screen);
	draw_score(screen);

	if (SDL_Flip(screen) == -1) {
		fprintf(stderr, "SDL_Flip failed.\n");
		return 1;
	}

	return 0;
}

static void rotate_row(int row)
{
	int i;
	enum spot tmp;

	assert(row >= 0);
	assert(row < BOARD_HEIGHT);

	tmp = board[BOARD_WIDTH - 1][row];
	for (i = BOARD_WIDTH - 1; i > 0; i--) {
		board[i][row] = board[i - 1][row];
	}
	board[0][row] = tmp;

	moving_row = row;
	horizontal_delta = -32;
	pieces_moving = 1;
}

static void invert_column(int col)
{
	int i;

	assert(col >= 0);
	assert(col < BOARD_WIDTH);

	for (i = 0; i < BOARD_HEIGHT; i++) {
		if (board[col][i] == WHITE)
			board[col][i] = BLACK;
		else if (board[col][i] == BLACK)
			board[col][i] = WHITE;
	}

	vertical_rotation = 1;
	moving_col = col;
	pieces_moving = 1;
}

void blow_up_block(int i, int j)
{
	enum spot spot = board[i][j];
	generate_particle(i * 32, j * 32, rand() % 2 ? 1 : -1, rand() % 2 ? 1 : -1, spot);
	generate_particle(i * 32 + 16, j * 32, rand() % 2 ? 1 : -1, rand() % 2 ? 1 : -1, spot);
	generate_particle(i * 32, j * 32 + 16, rand() % 2 ? 1 : -1, rand() % 2 ? 1 : -1, spot);
	generate_particle(i * 32 + 16, j * 32 + 16, rand() % 2 ? 1 : -1, rand() % 2 ? 1 : -1, spot);
}

int figure_out_completed_space(int i, int j)
{
	int *a1 = &board[i - 1][j - 1];
	int *a2 = &board[i - 1][j - 0];
	int *a3 = &board[i - 1][j + 1];
	int *b1 = &board[i - 0][j - 1];
	int *b2 = &board[i - 0][j - 0];
	int *b3 = &board[i - 0][j + 1];
	int *c1 = &board[i + 1][j - 1];
	int *c2 = &board[i + 1][j - 0];
	int *c3 = &board[i + 1][j + 1];

	if ((*a1 & EMPTY) || (*a2 & EMPTY) || (*a3 & EMPTY)
	    || (*b1 & EMPTY) || (*b2 & EMPTY) || (*b3 & EMPTY)
	    || (*c1 & EMPTY) || (*c2 & EMPTY) || (*c3 & EMPTY))
		return 0;

	if ((*a1 & FALLING) || (*a2 & FALLING) || (*a3 & FALLING)
	    || (*b1 & FALLING) || (*b2 & FALLING) || (*b3 & FALLING)
	    || (*c1 & FALLING) || (*c2 & FALLING) || (*c3 & FALLING))
		return 0;

	if (*a1 == *a2 && *a2 == *a3 && *a3 == *b1
	    && *b1 == *b2 && *b2 == *b3 && *b3 == *c1
	    && *c1 == *c2 && *c2 == *c3) {
		blow_up_block(i - 1, j - 1);
		blow_up_block(i - 1, j + 0);
		blow_up_block(i - 1, j + 1);
		blow_up_block(i + 0, j - 1);
		blow_up_block(i + 0, j + 0);
		blow_up_block(i + 0, j + 1);
		blow_up_block(i + 1, j - 1);
		blow_up_block(i + 1, j + 0);
		blow_up_block(i + 1, j + 1);

		*a1 = *a2 = *a3 = EMPTY;
		*b1 = *b2 = *b3 = EMPTY;
		*c1 = *c2 = *c3 = EMPTY;

		score += 100;

		return 1;
	}

	return 0;
}

static int figure_out_completed()
{
	int i;
	int j;
	int modified = 0;

	if (pieces_moving)
		return 0;

	for (i = 1; i < BOARD_WIDTH - 1; i++) {
		for (j = 1; j < BOARD_HEIGHT - 1; j++) {
			modified |= figure_out_completed_space(i, j);
		}
	}

	return modified;
}

/* Returns 1 if there a piece somewhere above this one. */
static int piece_above(int pi, int pj)
{
	int j;

	for (j = pj - 1; j > 0; j--) {
		if (board[pi][j] != EMPTY)
			return 1;
	}

	return 0;
}

static void handle_gravity_for_piece(int i, int j)
{
	int j2;

	if (board[i][j] != EMPTY)
		return;

	if (!piece_above(i, j))
		return;

	for (j2 = j; j2 > 0; j2--) {
		if (!(board[i][j2] & EMPTY)
		    && !(board[i][j2] & FALLING)) {
			board[i][j2] |= FALLING;
			board_deltas[i][j2] = 0;
			board_fall_times[i][j2] = HOLD_TIME;
			pieces_moving = 1;
		}
	}
}

static void handle_gravity()
{
	int i;
	int j;

	for (i = BOARD_WIDTH - 1; i >= 0; i--) {
		for (j = BOARD_HEIGHT - 1; j > 0; j--) {
			handle_gravity_for_piece(i, j);
		}
	}
}

/* Returns 1 if the piece is falling. */
static int handle_falling_piece(int i, int j, float dt)
{
	if (!(board[i][j] & FALLING))
		return 0;

	if (board_fall_times[i][j] > 0) {
		board_fall_times[i][j] -= 10 * dt;
		return 1;
	}

	if (board_deltas[i][j] > -0.10) {
		if (j == BOARD_HEIGHT - 1) {
			board[i][j] &= ~FALLING;
			board_deltas[i][j] = 0;
			return 0;
		} else if (board[i][j + 1] & EMPTY) {
			board[i][j + 1] = board[i][j];
			board[i][j] = EMPTY;
			board_deltas[i][j + 1] = -32;
			board_deltas[i][j] = 0;
			return 1;
		} else {
			board[i][j] &= ~FALLING;
			board_deltas[i][j] = 0;
			return 0;
		}
	} else {
		board_deltas[i][j] += FALL_SPEED * dt;
		if (board_deltas[i][j] >= 0)
			board_deltas[i][j] = 0;
		return 1;
	}
}

static void handle_falling(float dt)
{
	int i;
	int j;
	int falling = 0;

	for (i = BOARD_WIDTH - 1; i >= 0; i--) {
		for (j = BOARD_HEIGHT - 1; j > 0; j--) {
			falling |= handle_falling_piece(i, j, dt);
		}
	}

	if (pieces_moving)
		pieces_moving = falling;
}

void print_fps(int frames, Uint32 start)
{
	unsigned int total_ticks, fps;

	total_ticks = SDL_GetTicks() - start;
	if (total_ticks > 1000)
		fps = frames / (total_ticks / 1000);
	else
		fps = 0;
	printf("FPS: %u (%u)\n", fps, SDL_GetTicks());
}

void handle_mouse(const SDL_Event *event)
{
	/* User cannot do anything while board in motion. */
	if (pieces_moving)
		return;

	if (event->button.button == SDL_BUTTON_RIGHT) {
		invert_column(event->button.x / 32);
	} else if (event->button.button == SDL_BUTTON_LEFT) {
		rotate_row((event->button.y + new_row_delta) / 32);
	}
}

void init_board()
{
	int i, j;

	memset(board_fall_times, 0, sizeof(board_fall_times));
	memset(board_deltas, 0, sizeof(board_deltas));
	memset(new_row, 0, sizeof(new_row));
	new_row_delta = 0;

	pieces_moving = 0;

	score = 0;
	moving_col = -1;
	moving_row = -1;
	vertical_rotation = 0;
	horizontal_delta = 0;

	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = 0; j < BOARD_HEIGHT; j++)
			board[i][j] = EMPTY;

	for (i = 0; i < BOARD_WIDTH; i++)
		for (j = BOARD_HEIGHT - 10; j < BOARD_HEIGHT; j++)
			board[i][j] = rand() % 2 ? WHITE : BLACK;

	for (i = 0; i < BOARD_WIDTH; i++)
		new_row[i] = rand() % 2 ? WHITE : BLACK;
}

void update(float dt)
{
	int prev_pieces_moving = pieces_moving;

	handle_falling(dt);

	if (moving_col != -1) {
		pieces_moving = 1;

		if (vertical_rotation > 0) {
			vertical_rotation += 32 * 12 * dt;
		}

		if (vertical_rotation > 120) {
			vertical_rotation = 0;
			moving_col = -1;
			pieces_moving = 0;
		}
	}

	if (moving_row != -1) {
		pieces_moving = 1;

		if (horizontal_delta < 0) {
			horizontal_delta += 32 * 5 * dt;
			if (horizontal_delta >= 0)
				horizontal_delta = 0;
		} else {
			horizontal_delta = 0;
			moving_row = -1;
			pieces_moving = 0;
		}
	}

	if (prev_pieces_moving && !pieces_moving) {
		figure_out_completed();
		handle_gravity();
	}

	if (!pieces_moving) {
		if (new_row_delta < 32) {
			new_row_delta += 5 * dt;
		} else if (new_row_delta > 32) {
			add_new_row();
		}
	}

	update_particles(dt);
}

int main(int argc, char **argv)
{
	int quit = 0;
	Uint32 start = 0;
	int frames = 0;
	int last_tick = 0;
	int this_tick = 0;
	int ticks = 0;
	SDL_Surface *screen;
	SDL_Event event;
	float dt;

	(void) argc;
	(void) argv;

	if (init(&screen) != 0) {
		fprintf(stderr, "init failed.\n");
		return 1;
	}

	load_files();
	init_board();

	start = SDL_GetTicks();
	last_tick = start;

	while (quit == 0) {
		this_tick = SDL_GetTicks();
		ticks = this_tick - last_tick;
		last_tick = this_tick;

		dt = ticks / 1000.0;

		update(dt);

		if (draw(screen) != 0) {
			fprintf(stderr, "draw failed\n");
			return 1;
		}

		while (SDL_PollEvent(&event)) {

			switch (event.type) {
			case SDL_QUIT:
				quit = 1;
				break;
			case SDL_MOUSEBUTTONUP:
				handle_mouse(&event);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
				case SDLK_q:
					quit = 1;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
		frames++;
	}

	print_fps(frames, start);

	clean_up();

	return 0;
}
