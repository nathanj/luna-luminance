#include <errno.h>
#include <assert.h>
#include <math.h>

#include "SDL.h"
#include "SDL/SDL_image.h"

#include "list.h"

enum spot {
	EMPTY   = 0x01,
	BLACK   = 0x02,
	WHITE   = 0x04,

	DESTROY = 0x10,
	FALLING = 0x20,
};

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 15

struct particle {
	float x, y, dx, dy;
	SDL_Surface *image;
	float t;
	float decay;

	struct list_head list;
};

struct images {
	SDL_Surface *background;
	SDL_Surface *black_image;
	SDL_Surface *black_to_white[4];
	SDL_Surface *black_scale[3];
	SDL_Surface *white_scale[3];
	SDL_Surface *white_image;
	SDL_Surface *cursor_image;
	SDL_Surface *font;
};

extern struct images images;
extern SDL_Surface *screen;

SDL_Surface *load_image(const char *filename);
void apply_surface(int x, int y, SDL_Surface *source,
		   SDL_Surface *destination, SDL_Rect *clip);

struct particle *make_particle(float x, float y, float dx, float dy,
			       enum spot spot);
void generate_particle(float x, float y, float dx, float dy,
		       enum spot spot);
void draw_particles(SDL_Surface *screen);
void update_particles();
