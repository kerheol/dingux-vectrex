#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>

#include "global.h"
#include "psp_sdl.h"
#include "psp_dve.h"
#include "osint.h"
#include "vecx.h"
#include "bios.h"
#include "wnoise.h"            // White noise waveform

#define video_buffer_pitch BLIT_WIDTH

static unsigned short color_set_blue[VECTREX_COLORS];
static unsigned short color_set_gray[VECTREX_COLORS];
static unsigned short color_set_red[VECTREX_COLORS];
static unsigned short color_set_green[VECTREX_COLORS];
static unsigned short color_set_yellow[VECTREX_COLORS];
static unsigned short *color_set;

# define scl_factor 136

static void
osint_gencolors (void)
{
	int c;
	unsigned char rcomp, gcomp, bcomp;

	for (c = 0; c < VECTREX_COLORS; c++) {
		rcomp = c * 256 / VECTREX_COLORS;
		gcomp = c * 256 / VECTREX_COLORS;
		bcomp = c * 256 / VECTREX_COLORS;
	    color_set_gray[c] = SDL_MapRGB(back_surface->format,rcomp,gcomp,bcomp);
		rcomp = c * 50 / VECTREX_COLORS;
		gcomp = c * 50 / VECTREX_COLORS;
		bcomp = c * 256 / VECTREX_COLORS;
	    color_set_blue[c] = SDL_MapRGB(back_surface->format,rcomp,gcomp,bcomp);
		rcomp = c * 256 / VECTREX_COLORS;
		gcomp = c * 50 / VECTREX_COLORS;
		bcomp = c * 50 / VECTREX_COLORS;
	    color_set_red[c] = SDL_MapRGB(back_surface->format,rcomp,gcomp,bcomp);
		rcomp = c * 50 / VECTREX_COLORS;
		gcomp = c * 256 / VECTREX_COLORS;
		bcomp = c * 50 / VECTREX_COLORS;
	    color_set_green[c] = SDL_MapRGB(back_surface->format,rcomp,gcomp,bcomp);
		rcomp = c * 256 / VECTREX_COLORS;
		gcomp = c * 256 / VECTREX_COLORS;
		bcomp = c * 50 / VECTREX_COLORS;
	    color_set_yellow[c] = SDL_MapRGB(back_surface->format,rcomp,gcomp,bcomp);
	}
  color_set = color_set_gray;
}

void
osint_set_color(int color)
{
	switch(color){
		case COLOR_BLUE:
			color_set = color_set_blue;
			break;
		case COLOR_RED:
			color_set = color_set_red;
			break;
		case COLOR_GREEN:
			color_set = color_set_green;
			break;
		case COLOR_YELLOW:
			color_set = color_set_yellow;
			break;
		case COLOR_GRAY:
			color_set = color_set_gray;
			break;
	}
}

static inline unsigned short *
osint_pixelptr (long x, long y)
{
	unsigned short *ptr;
	ptr = blit_surface->pixels;
	ptr += y * video_buffer_pitch;
	ptr += x;

	return ptr;
}

/* draw a line with a slope between 0 and 1.
 * x is the "driving" axis. x0 < x1 and y0 < y1.
 */

static void osint_linep01 (long x0, long y0, long x1, long y1, short color)
{
	long dx, dy;
	long i0, i1;
	long j, e;
	unsigned short *ptr;

	dx = x1 - x0;
	dy = y1 - y0;

	i0 = x0 / scl_factor;
	i1 = x1 / scl_factor;
	j  = y0 / scl_factor;

	e = dy * (scl_factor - (x0 % scl_factor)) -
		  dx * (scl_factor - (y0 % scl_factor));

	dx *= scl_factor;
	dy *= scl_factor;

	ptr = osint_pixelptr (i0, j);

	for (; i0 <= i1; i0++) {
    *ptr++ = color;
		if (e >= 0) {
			ptr += video_buffer_pitch;
			e -= dx;
		}
		e += dy;
	}
}

/* draw a line with a slope between 1 and +infinity.
 * y is the "driving" axis. y0 < y1 and x0 < x1.
 */

static void osint_linep1n (long x0, long y0, long x1, long y1, short color)
{
	long dx, dy;
	long i0, i1;
	long j, e;
	unsigned short *ptr;

	dx = x1 - x0;
	dy = y1 - y0;

	i0 = y0 / scl_factor;
	i1 = y1 / scl_factor;
	j  = x0 / scl_factor;

	e = dx * (scl_factor - (y0 % scl_factor)) -
		  dy * (scl_factor - (x0 % scl_factor));

	dx *= scl_factor;
	dy *= scl_factor;

	ptr = osint_pixelptr (j, i0);

	for (; i0 <= i1; i0++) {
    *ptr = color;

		if (e >= 0) {
			ptr++;
			e -= dy;
		}

		e += dx;
		ptr += video_buffer_pitch;
	}
}

/* draw a line with a slope between 0 and -1.
 * x is the "driving" axis. x0 < x1 and y1 < y0.
 */

static void osint_linen01 (long x0, long y0, long x1, long y1, short color)
{
	long dx, dy;
	long i0, i1;
	long j, e;
	unsigned short *ptr;

	dx = x1 - x0;
	dy = y0 - y1;

	i0 = x0 / scl_factor;
	i1 = x1 / scl_factor;
	j  = y0 / scl_factor;

	e = dy * (scl_factor - (x0 % scl_factor)) -
		  dx * (y0 % scl_factor);

	dx *= scl_factor;
	dy *= scl_factor;

	ptr = osint_pixelptr (i0, j);

	for (; i0 <= i1; i0++) {
    *ptr = color;

		if (e >= 0) {
			ptr -= video_buffer_pitch;
			e -= dx;
		}

		e += dy;
		ptr++;
	}
}

/* draw a line with a slope between -1 and -infinity.
 * y is the "driving" axis. y0 < y1 and x1 < x0.
 */

static void osint_linen1n (long x0, long y0, long x1, long y1, short color)
{
	long dx, dy;
	long i0, i1;
	long j, e;
	unsigned short *ptr;

	dx = x0 - x1;
	dy = y1 - y0;

	i0 = y0 / scl_factor;
	i1 = y1 / scl_factor;
	j  = x0 / scl_factor;

	e = dx * (scl_factor - (y0 % scl_factor)) -
		dy * (x0 % scl_factor);

	dx *= scl_factor;
	dy *= scl_factor;

	ptr = osint_pixelptr (j, i0);

	for (; i0 <= i1; i0++) {
    *ptr = color;
		if (e >= 0) {
			ptr--;
			e -= dy;
		}

		e += dx;
		ptr += video_buffer_pitch;
	}
}

static void osint_line (point_t* p0, point_t* p1, short color)
{
  long x0;
  long y0;
  long x1;
  long y1;

  if (DVE.dve_render_mode == DVE_RENDER_ROT90) {
    x0 = ALG_MAX_Y - p0->y; y0 = p0->x; x1 = ALG_MAX_Y - p1->y; y1 = p1->x;
  } else {
    x0 = p0->x; y0 = p0->y; x1 = p1->x; y1 = p1->y;
  }

	if (x1 > x0) {
		if (y1 > y0) {
			if ((x1 - x0) > (y1 - y0)) {
				osint_linep01 (x0, y0, x1, y1, color);
			} else {
				osint_linep1n (x0, y0, x1, y1, color);
			}
		} else {
			if ((x1 - x0) > (y0 - y1)) {
				osint_linen01 (x0, y0, x1, y1, color);
			} else {
				osint_linen1n (x1, y1, x0, y0, color);
			}
		}
	} else {
		if (y1 > y0) {
			if ((x0 - x1) > (y1 - y0)) {
				osint_linen01 (x1, y1, x0, y0, color);
			} else {
				osint_linen1n (x0, y0, x1, y1, color);
			}
		} else {
			if ((x0 - x1) > (y0 - y1)) {
				osint_linep01 (x1, y1, x0, y0, color);
			} else {
				osint_linep1n (x1, y1, x0, y0, color);
			}
		}
	}
}

void
osint_video_reset(void)
{
  short *ptr = blit_surface->pixels;
  int len = blit_surface->w * blit_surface->h;
  while (len--) {
    *ptr++ = 0;
  }
}

static int loc_render_count = 0;

void
osint_render(void)
{
  if (++loc_render_count > 30) {
    osint_video_reset();
    loc_render_count = 0;
  }

  int v;
  for (v = 0; v < vector_erse_cnt; v++) {
    unsigned short color = vectors_erse[v].color;
    if (color & 0x80) continue;
    osint_line (&vectors_erse[v].p0, &vectors_erse[v].p1, 0);
  }

	for (v = 0; v < vector_draw_cnt; v++) {
    unsigned char color = vectors_draw[v].color;
    if (color & 0x80) continue;
    unsigned short color_pix = color_set[color];

		osint_line (&vectors_draw[v].p0, &vectors_draw[v].p1, color_pix);
	}
}



int
dve_loadcart(const char *filename)
{
  FILE *f;
  unsigned int filesize;

  f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Can't open cartridge image (%s).\n", filename);
    return (1);
  }

  fseek(f, 0, SEEK_END);
  filesize = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (filesize > sizeof(cart)) {
    fprintf(
      stderr,
      "%s is not a valid Vectrex cartridge.\n"
      "It's larger that %d bytes.\n",
      filename, sizeof(cart)
    );
    fclose(f);
    return (1);
  }

  if (filesize != fread(cart, 1, filesize, f)) {
    fprintf(stderr, "Error while reading %s.\n", filename);
    fclose(f);
    return (1);
  }

  fclose(f);
  return 0;
}

int
SDL_main(int argc, char* argv[])
{
  memcpy(rom, bios, sizeof(rom));
  memset(cart, 0, sizeof(cart));

  dve_initialize();

  osint_gencolors();
  e8910_init_sound();

  vecx_reset();

  psp_sdl_black_screen();

  vecx_emu_loop();

  return 0;
}
