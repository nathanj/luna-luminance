#include "game.h"

LIST_HEAD(particle_list);

struct particle *make_particle(float x, float y, float dx, float dy,
			       enum spot spot)
{
	struct particle *particle;

	particle = malloc(sizeof(*particle));
	particle->x = x;
	particle->y = y;
	particle->dx = dx * (rand() % 10) / 20.0;
	particle->dy = dy * (rand() % 10) / 20.0;

	particle->image = ((spot & 0xFF) == WHITE)
		? images.white_scale[0] : images.black_scale[0];
	particle->t = 0;
	particle->decay = (fabs(particle->dx) + fabs(particle->dy)) * 250;

	return particle;
}

void generate_particle(float x, float y, float dx, float dy,
		       enum spot spot)
{
	list_add_tail(&(make_particle(x, y, dx, dy, spot)->list),
		      &particle_list);
}

void draw_particles(SDL_Surface *screen)
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
			if (p->image == images.white_scale[0])
				p->image = images.white_scale[1];
			if (p->image == images.black_scale[0])
				p->image = images.black_scale[1];
		} else if (p->t > p->decay*2 && p->t <= p->decay*3) {
			if (p->image == images.white_scale[1])
				p->image = images.white_scale[2];
			if (p->image == images.black_scale[1])
				p->image = images.black_scale[2];
		} else if (p->t > p->decay*3) {
			list_del(&p->list);
			free(p);
		}
	}
}

