#include "game.h"

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

