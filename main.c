#include <errno.h>
#include <assert.h>
#include <math.h>

#include "SDL.h"
#include "SDL/SDL_image.h"

#include "list.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;

SDL_Surface *background = NULL;
SDL_Surface *black_image = NULL;
SDL_Surface *black_to_white[4] = {NULL};
SDL_Surface *black_scale[3] = {NULL};
SDL_Surface *white_scale[3] = {NULL};
SDL_Surface *white_image = NULL;
SDL_Surface *cursor_image = NULL;
SDL_Surface *screen = NULL;

SDL_Event event;

SDL_Surface *font = NULL;

enum spot {
	EMPTY   = 0x01,
	BLACK   = 0x02,
	WHITE   = 0x04,

	DESTROY = 0x10,
	FALLING = 0x20,
};

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 15
int board[BOARD_WIDTH][BOARD_HEIGHT];
int board_fall_times[BOARD_WIDTH][BOARD_HEIGHT];
int board_deltas[BOARD_WIDTH][BOARD_HEIGHT];

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

struct particle {
	float x, y, dx, dy;
	SDL_Surface *image;
	float t;
	float decay;

	struct list_head list;
};

LIST_HEAD(particle_list);

#define HOLD_TIME 20
#define FALL_SPEED 16

struct particle *make_particle(float x, float y, float dx, float dy, enum spot spot)
{
	struct particle *particle;

	particle = malloc(sizeof(*particle));
	particle->x = x;
	particle->y = y;
	particle->dx = dx * (rand() % 10) / 20.0;
	particle->dy = dy * (rand() % 10) / 20.0;

	particle->image = ((spot & 0xFF) == WHITE) ? white_scale[0] : black_scale[0];
	particle->t = 0;
	particle->decay = (fabs(particle->dx) + fabs(particle->dy)) * 250;

	return particle;
}

void generate_particle(float x, float y, float dx, float dy, enum spot spot)
{
	list_add_tail(&(make_particle(x, y, dx, dy, spot)->list), &particle_list);
}

SDL_Surface *load_image(const char *filename)
{
	SDL_Surface *loadedImage = NULL;
	SDL_Surface *optimizedImage = NULL;

	loadedImage = IMG_Load(filename);

	if (loadedImage == NULL) {
		fprintf(stderr, "Could not load %s: %s\n",
			filename, strerror(errno));
		assert(0);
	}

	optimizedImage = SDL_DisplayFormatAlpha(loadedImage);
	SDL_FreeSurface(loadedImage);

	if (optimizedImage == NULL) {
		fprintf(stderr, "Could not optimize image: %s\n",
			filename);
		assert(0);
	}

	return optimizedImage;
}

void apply_surface(int x, int y, SDL_Surface *source,
		   SDL_Surface *destination, SDL_Rect *clip)
{
	SDL_Rect offset;

	offset.x = x;
	offset.y = y;

	SDL_BlitSurface(source, clip, destination, &offset);
}

int init()
{
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		fprintf(stderr, "SDL_Init failed.\n");
		return 1;
	}

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT,
				  SCREEN_BPP,
				  SDL_SWSURFACE);

	if(screen == NULL) {
		fprintf(stderr, "SDL_SetVideoMode failed.\n");
		return 1;
	}

	SDL_WM_SetCaption("Puzzle Game", NULL);

	return 0;
}

void load_files()
{
	black_image = load_image("assets/black.png");
	white_image = load_image("assets/white.png");

	black_to_white[0] = load_image("assets/bw1.png");
	black_to_white[1] = load_image("assets/bw2.png");
	black_to_white[2] = load_image("assets/bw3.png");
	black_to_white[3] = load_image("assets/bw4.png");

	black_scale[0] = load_image("assets/black_particle1.png");
	black_scale[1] = load_image("assets/black_particle2.png");
	black_scale[2] = load_image("assets/black_particle3.png");
	white_scale[0] = load_image("assets/white_particle1.png");
	white_scale[1] = load_image("assets/white_particle2.png");
	white_scale[2] = load_image("assets/white_particle3.png");

	background = load_image("assets/space.png");

	font = load_image("assets/font.png");
}

void clean_up()
{
	SDL_FreeSurface(white_image);
	SDL_FreeSurface(black_image);

	SDL_Quit();
}

int update(int ticks)
{
	(void)ticks;
	//sprite_update(&sprite, ticks);

	return 0;
}

static void draw_new_piece(int i, int dy, SDL_Rect *clip)
{
	switch (new_row[i] & 0x0F) {
	case BLACK:
		apply_surface(i * 32, 480 + dy,
			      black_image, screen,
			      clip);
		break;
	case WHITE:
		apply_surface(i * 32, 480 + dy,
			      white_image, screen,
			      clip);
		break;
	default:
		break;
	}
}

static void draw_piece(int i, int j, int dx, int dy, SDL_Rect *clip)
{
	int rot = -1;

	if (moving_col == i)
		rot = vertical_rotation;

	switch (board[i][j] & 0x0F) {
	case BLACK:
		if (rot > 0 && rot <= 30)
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_to_white[3], screen,
				      clip);
		else if (rot > 30 && rot <= 60)
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_to_white[2], screen,
				      clip);
		else if (rot > 60 && rot <= 90)
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_to_white[1], screen,
				      clip);
		else if (rot > 90 && rot <= 120)
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_to_white[0], screen,
				      clip);
		else
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_image, screen,
				      clip);
		break;
	case WHITE:
		if (rot > 0 && rot <= 30)
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_to_white[0], screen,
				      clip);
		else if (rot > 30 && rot <= 60)
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_to_white[1], screen,
				      clip);
		else if (rot > 60 && rot <= 90)
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_to_white[2], screen,
				      clip);
		else if (rot > 90 && rot <= 120)
			apply_surface(i * 32 + dx, j * 32 + dy,
				      black_to_white[3], screen,
				      clip);
		else
			apply_surface(i * 32 + dx, j * 32 + dy,
				      white_image, screen,
				      clip);
		break;
	default:
		break;
	}
}

static void draw_board()
{
	int i;
	int j;
	SDL_Rect clip;
	int dy;
	int dx;

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

			if (dx != 0 && i == 0) {
				clip.w = 32;
				draw_piece(i, j, dx, dy, &clip);

				clip.w = -dx;
				dx += 10 * 32;
				draw_piece(i, j, dx, dy, &clip);
			} else {
				clip.w = 32;
				draw_piece(i, j, dx, dy, &clip);
			}
		}
	}

	clip.w = 32;
	clip.h = -new_row_delta;
	dy = -new_row_delta;

	for (i = 0; i < BOARD_WIDTH; i++) {
		draw_new_piece(i, dy, &clip);
	}
}

void draw_particles()
{
	struct particle *p;

	list_for_each_entry(p, &particle_list, list) {
		apply_surface(p->x, p->y,
			      p->image, screen,
			      NULL);
	}
}

void update_particles()
{
	struct particle *p, *n;

	list_for_each_entry_safe(p, n, &particle_list, list) {
		p->t++;
		p->x += p->dx;
		p->y += p->dy;


		if (p->t > p->decay && p->t <= p->decay*2) {
			if (p->image == white_scale[0])
				p->image = white_scale[1];
			if (p->image == black_scale[0])
				p->image = black_scale[1];
		} else if (p->t > p->decay*2 && p->t <= p->decay*3) {
			if (p->image == white_scale[1])
				p->image = white_scale[2];
			if (p->image == black_scale[1])
				p->image = black_scale[2];
		} else if (p->t > p->decay*3) {
			list_del(&p->list);
			free(p);
		}
	}
}

void draw_score()
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
		apply_surface(x, 100, font, screen, &clip);
		return;
	}

	while (s > 0) {
		c = s % 10;
		s /= 10;
		clip.x = c * 32;
		apply_surface(x, 100, font, screen, &clip);
		x -= 32;
	}
}

int draw()
{
	apply_surface(0, 0, background, screen, NULL);

	draw_board();
	draw_particles();
	draw_score();

	if(SDL_Flip(screen) == -1) {
		fprintf(stderr, "SDL_Flip failed.\n");
		return 1;
	}

	return 0;
}

static void print_board()
{
	int i;
	int j;

	for (j = 0; j < BOARD_HEIGHT; j++) {
		for (i = 0; i < BOARD_WIDTH; i++) {
			switch (board[i][j] & 0xF0) {
			case FALLING:
				printf("F");
				break;
			case DESTROY:
				printf("D");
				break;
			default:
				printf(" ");
				break;
			}
			switch (board[i][j] & 0x0F) {
			case EMPTY:
				printf(" ");
				break;
			case BLACK:
				printf("B");
				break;
			case WHITE:
				printf("W");
				break;
			default:
				printf("X");
				break;
			}
		}
		printf("\n");
	}
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

	printf("col = %d\n", col);

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
		if (!(board[i][j2] & EMPTY)) {
			board[i][j2] |= FALLING;
			board_deltas[i][j2] = 0;
			if (board_fall_times[i][j2] == 0)
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

static void handle_falling_piece(int i, int j)
{
	if (!(board[i][j] & FALLING))
		return;

	if (board_fall_times[i][j] > 0) {
		board_fall_times[i][j] -= 10;
		pieces_moving = 1;
		return;
	}

	if (board_deltas[i][j] == 0) {
		if (j == BOARD_HEIGHT - 1) {
			board[i][j] &= ~FALLING;
			board_deltas[i][j] = 0;
		} else if (board[i][j + 1] & EMPTY) {
			board[i][j + 1] = board[i][j];
			board[i][j] = EMPTY;
			board_deltas[i][j + 1] = -32;
			pieces_moving = 1;
		} else {
			board[i][j] &= ~FALLING;
			board_deltas[i][j] = 0;
		}
	} else {
		board_deltas[i][j] += FALL_SPEED;
		pieces_moving = 1;
	}
}

static void handle_falling()
{
	int i;
	int j;

	pieces_moving = 0;
	for (i = BOARD_WIDTH - 1; i >= 0; i--) {
		for (j = BOARD_HEIGHT - 1; j > 0; j--) {
			handle_falling_piece(i, j);
		}
	}
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

int main(int argc, char **argv)
{
	int quit = 0;
	Uint32 start = 0;
	int frames = 0;
	int last_tick = 0;
	int this_tick = 0;
	int ticks = 0;
	int i;
	int j;
	int frame = 0;

	(void) argc;
	(void) argv;

	if(init() != 0) {
		fprintf(stderr, "init failed.\n");
		return 1;
	}

	load_files();

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

	print_board();
	//rotate_row(BOARD_HEIGHT - 3);

	start = SDL_GetTicks();
	last_tick = start;

	while (quit == 0) {
		this_tick = SDL_GetTicks();
		ticks = this_tick - last_tick;
		last_tick = this_tick;

		frame++;
		if (frame % 10 == 0) {
			handle_falling();
			while (figure_out_completed()) {
				handle_gravity();
			}
		}

		if (update(ticks) != 0) {
			fprintf(stderr, "update failed\n");
			return 1;
		}

		if (draw() != 0) {
			fprintf(stderr, "draw failed\n");
			return 1;
		}

		if (moving_col != -1) {
			if (vertical_rotation > 0) {
				vertical_rotation += 1;
			} else if (vertical_rotation > 120) {
				vertical_rotation = 0;
				moving_col = -1;
				pieces_moving = 0;
			}
		}

		if (moving_row != -1) {
			if (horizontal_delta < 0) {
				horizontal_delta += 1.0;
			} else {
				horizontal_delta = 0;
				moving_row = -1;
				pieces_moving = 0;
			}
		}

		if (!pieces_moving) {
			if (new_row_delta < 32) {
				new_row_delta += 0.01;
			} else if (new_row_delta > 32) {
				add_new_row();
			}
		}

		update_particles();

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_MOUSEBUTTONUP) {
				if (!pieces_moving) {
					printf("click at %d %d\n", event.button.x / 32, event.button.y / 32);
					if (event.button.button == SDL_BUTTON_RIGHT) {
						invert_column(event.button.x / 32);
					} else if (event.button.button == SDL_BUTTON_LEFT) {
						rotate_row((event.button.y + new_row_delta) / 32);
					}

					figure_out_completed();
					handle_gravity();
				}
			}

			switch (event.type) {
			case SDL_QUIT:
				quit = 1;
				break;
			case SDL_KEYDOWN:
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
