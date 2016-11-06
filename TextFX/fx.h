/*
 * blur
 *
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2009 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * map-to-path
 *
 * Written by Behdad Esfahbod, 2006..2007
 * 
 * Permission to use, copy, modify, distribute, and sell this example
 * for any purpose is hereby granted without fee.
 * It is provided "as is" without express or implied warranty.
 *
 */
#ifndef FX_H
#define FX_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

/* Performs a simple 2D Gaussian blur of radius @radius on surface @surface. */
inline void
blur_image_surface (cairo_surface_t *surface, int radius)
{
    cairo_surface_t *tmp;
    int width, height;
    int src_stride, dst_stride;
    int x, y, z, w;
    uint8_t *src, *dst;
    uint32_t *s, *d, a, p;
    int i, j, k;
    uint8_t kernel[17];
    const int size = ARRAY_LENGTH (kernel);
    const int half = size / 2;

    if (cairo_surface_status (surface))
        return;

    width = cairo_image_surface_get_width (surface);
    height = cairo_image_surface_get_height (surface);

    switch (cairo_image_surface_get_format (surface))
    {
    case CAIRO_FORMAT_A1:
    default:
        /* Don't even think about it! */
        return;

    case CAIRO_FORMAT_A8:
        /* Handle a8 surfaces by effectively unrolling the loops by a
         * factor of 4 - this is safe since we know that stride has to be a
         * multiple of uint32_t. */
        width /= 4;
        break;

    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_ARGB32:
        break;
    }

    tmp = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status (tmp))
        return;

    src = cairo_image_surface_get_data (surface);
    src_stride = cairo_image_surface_get_stride (surface);

    dst = cairo_image_surface_get_data (tmp);
    dst_stride = cairo_image_surface_get_stride (tmp);

    a = 0;
    for (i = 0; i < size; i++)
    {
        double f = i - half;
        a += kernel[i] = exp (- f * f / 30.0) * 80;
    }

    /* Horizontally blur from surface -> tmp */
    for (i = 0; i < height; i++)
    {
        s = (uint32_t *) (src + i * src_stride);
        d = (uint32_t *) (dst + i * dst_stride);
        for (j = 0; j < width; j++)
        {
            if (radius < j && j < width - radius)
            {
                d[j] = s[j];
                continue;
            }

            x = y = z = w = 0;
            for (k = 0; k < size; k++)
            {
                if (j - half + k < 0 || j - half + k >= width)
                    continue;

                p = s[j - half + k];

                x += ((p >> 24) & 0xff) * kernel[k];
                y += ((p >> 16) & 0xff) * kernel[k];
                z += ((p >>  8) & 0xff) * kernel[k];
                w += ((p >>  0) & 0xff) * kernel[k];
            }
            d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
        }
    }

    /* Then vertically blur from tmp -> surface */
    for (i = 0; i < height; i++)
    {
        s = (uint32_t *) (dst + i * dst_stride);
        d = (uint32_t *) (src + i * src_stride);
        for (j = 0; j < width; j++)
        {
            if (radius <= i && i < height - radius)
            {
                d[j] = s[j];
                continue;
            }

            x = y = z = w = 0;
            for (k = 0; k < size; k++)
            {
                if (i - half + k < 0 || i - half + k >= height)
                    continue;

                s = (uint32_t *) (dst + (i - half + k) * dst_stride);
                p = s[j];

                x += ((p >> 24) & 0xff) * kernel[k];
                y += ((p >> 16) & 0xff) * kernel[k];
                z += ((p >>  8) & 0xff) * kernel[k];
                w += ((p >>  0) & 0xff) * kernel[k];
            }
            d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
        }
    }

    cairo_surface_destroy (tmp);
    cairo_surface_mark_dirty (surface);
}

/* Returns Euclidean distance between two points */
static inline double two_points_distance(cairo_path_data_t* a, cairo_path_data_t* b) {
	double dx, dy;

	dx = b->point.x - a->point.x;
	dy = b->point.y - a->point.y;

	return sqrt(dx * dx + dy * dy);
}

/* Returns length of a Bezier curve. Seems like computing that analytically is not easy. The
 * code just flattens the curve using cairo and adds the length of segments. */
static inline double curve_length(
	double x0,
	double y0,
	double x1,
	double y1,
	double x2,
	double y2,
	double x3,
	double y3
) {
	cairo_surface_t*   surface = cairo_image_surface_create(CAIRO_FORMAT_A8, 0, 0);
	cairo_t*           cr      = cairo_create(surface);
	double             length  = 0;
	cairo_path_t*      path;
	cairo_path_data_t* data;
	cairo_path_data_t  current_point;
	int                i;

	current_point.point.x = 0.0;
	current_point.point.y = 0.0;

	cairo_surface_destroy(surface);
	cairo_move_to(cr, x0, y0);
	cairo_curve_to(cr, x1, y1, x2, y2, x3, y3);

	path = cairo_copy_path_flat(cr);

	for(i = 0; i < path->num_data; i += path->data[i].header.length) {
		data = &path->data[i];

		switch (data->header.type) {
		case CAIRO_PATH_MOVE_TO:
			current_point = data[1];

			break;

		case CAIRO_PATH_LINE_TO:
			length        += two_points_distance(&current_point, &data[1]);
			current_point  = data[1];

			break;

		default:
			break;
		}
	}

	cairo_path_destroy(path);
	cairo_destroy(cr);

	return length;
}

typedef double parametrization_t;

/* Compute parametrization info. That is, for each part of the cairo path, tags it with
 * its length. */
static inline parametrization_t* parametrize_path(cairo_path_t* path) {
	parametrization_t* parametrization = 0;
	cairo_path_data_t* data            = 0;
	cairo_path_data_t  last_move_to;
	cairo_path_data_t  current_point;
	int                i;

	current_point.point.x = 0.0;
	current_point.point.y = 0.0;
	
	parametrization = (parametrization_t*)malloc(path->num_data * sizeof(parametrization[0]));

	for(i = 0; i < path->num_data; i += path->data[i].header.length) {
		data               = &path->data[i];
		parametrization[i] = 0.0;

		switch(data->header.type) {
		case CAIRO_PATH_MOVE_TO:
			last_move_to  = data[1];
			current_point = data[1];

			break;

		case CAIRO_PATH_CLOSE_PATH:
			/* Make it look like it's a line_to to last_move_to. */
			data = (&last_move_to) - 1;
			
		case CAIRO_PATH_LINE_TO:
			parametrization[i] = two_points_distance(&current_point, &data[1]);
			current_point      = data[1];

			break;

		case CAIRO_PATH_CURVE_TO:
			parametrization[i] = curve_length(
				current_point.point.x, current_point.point.x,
				data[1].point.x, data[1].point.y,
				data[2].point.x, data[2].point.y,
				data[3].point.x, data[3].point.y
			);

			current_point = data[3];

			break;
		}
	}

	return parametrization;
}

typedef void (*transform_point_func_t) (void *closure, double *x, double *y);

/* Project a path using a function. Each point of the path (including  Bezier control
 * points) is passed to the function for transformation. */
inline static void transform_path(cairo_path_t* path, transform_point_func_t f, void* closure) {
	cairo_path_data_t* data;
	int                i;

	for(i = 0; i < path->num_data; i += path->data[i].header.length) {
		data = &path->data[i];

		switch(data->header.type) {
		case CAIRO_PATH_CURVE_TO:
			f(closure, &data[3].point.x, &data[3].point.y);
			f(closure, &data[2].point.x, &data[2].point.y);

		case CAIRO_PATH_MOVE_TO:
		case CAIRO_PATH_LINE_TO:
			f(closure, &data[1].point.x, &data[1].point.y);

			break;

		case CAIRO_PATH_CLOSE_PATH:
			break;
		}
	}
}

typedef struct {
	cairo_path_t*      path;
	parametrization_t* parametrization;
} parametrized_path_t;

/* Project a point X,Y onto a parameterized path. The final point is
 * where you get if you walk on the path forward from the beginning for X
 * units, then stop there and walk another Y units perpendicular to the
 * path at that point. In more detail:
 *
 * There's three pieces of math involved:
 *
 *   - The parametric form of the Line equation
 *     http: *en.wikipedia.org/wiki/Line
 *
 *   - The parametric form of the Cubic Bézier curve equation
 *     http: *en.wikipedia.org/wiki/B%C3%A9zier_curve
 *
 *   - The Gradient (aka multi-dimensional derivative) of the above
 *     http: *en.wikipedia.org/wiki/Gradient
 *
 * The parametric forms are used to answer the question of "where will I be
 * if I walk a distance of X on this path". The Gradient is used to answer
 * the question of "where will I be if then I stop, rotate left for 90
 * degrees and walk straight for a distance of Y". */
inline static void point_on_path(parametrized_path_t* param, double* x, double* y) {
	double ratio;
	double the_y = *y;
	double the_x = *x;
	double dx;
	double dy;
	int    i;

	cairo_path_t*      path = param->path;
	parametrization_t* parametrization = param->parametrization;
	cairo_path_data_t* data;
	cairo_path_data_t  last_move_to;
	cairo_path_data_t  current_point;

	current_point.point.x = 0.0;
	current_point.point.y = 0.0;

	for(
		i = 0;
		i + path->data[i].header.length < path->num_data &&
		(the_x > parametrization[i] || path->data[i].header.type == CAIRO_PATH_MOVE_TO);
		i += path->data[i].header.length
	) {
		the_x -= parametrization[i];
		data   = &path->data[i];

		switch(data->header.type) {
		case CAIRO_PATH_MOVE_TO:
			current_point = data[1];
			last_move_to  = data[1];

			break;

		case CAIRO_PATH_LINE_TO:
			current_point = data[1];

			break;

		case CAIRO_PATH_CURVE_TO:
			current_point = data[3];

			break;

		case CAIRO_PATH_CLOSE_PATH:
			break;
		}
	}

	data = &path->data[i];

	switch(data->header.type) {
	case CAIRO_PATH_MOVE_TO:
		break;

	case CAIRO_PATH_CLOSE_PATH:
		/* Make it look like it's a line_to to last_move_to. */
		data = (&last_move_to) - 1;

	case CAIRO_PATH_LINE_TO:
		ratio = the_x / parametrization[i];
		/* Line polynomial */
		*x = current_point.point.x * (1 - ratio) + data[1].point.x * ratio;
		*y = current_point.point.y * (1 - ratio) + data[1].point.y * ratio;

		/* Line gradient */
		dx = -(current_point.point.x - data[1].point.x);
		dy = -(current_point.point.y - data[1].point.y);

		/* optimization for: ratio = the_y / sqrt (dx * dx + dy * dy); */
		ratio  = the_y / parametrization[i];
		*x    += -dy * ratio;
		*y    += dx * ratio;

		break;

	case CAIRO_PATH_CURVE_TO: {
		/* FIXME the formulas here are not exactly what we want, because the
		 * Bezier parametrization is not uniform. But I don't know how to do
		 * better. The caller can do slightly better though, by flattening the
		 * Bezier and avoiding this branch completely. That has its own cost
		 * though, as large y values magnify the flattening error drastically. */

		double ratio_1_0;
		double ratio_0_1;
		double ratio_2_0;
		double ratio_0_2;
		double ratio_3_0;
		double ratio_2_1;
		double ratio_1_2;
		double ratio_0_3;
		double _1__4ratio_1_0_3ratio_2_0;
		double _2ratio_1_0_3ratio_2_0;

		ratio     = the_x / parametrization[i];
		ratio_1_0 = ratio;
		ratio_0_1 = 1 - ratio;
		ratio_2_0 = ratio_1_0 * ratio_1_0;
		ratio_0_2 = ratio_0_1 * ratio_0_1;
		ratio_3_0 = ratio_2_0 * ratio_1_0;
		ratio_2_1 = ratio_2_0 * ratio_0_1;
		ratio_1_2 = ratio_1_0 * ratio_0_2;
		ratio_0_3 = ratio_0_1 * ratio_0_2;

		_1__4ratio_1_0_3ratio_2_0 = 1 - 4 * ratio_1_0 + 3 * ratio_2_0;
		_2ratio_1_0_3ratio_2_0    =     2 * ratio_1_0 - 3 * ratio_2_0;

		/* Bezier polynomial */
		*x = current_point.point.x * ratio_0_3
			+ 3 * data[1].point.x * ratio_1_2
			+ 3 * data[2].point.x * ratio_2_1
			+     data[3].point.x * ratio_3_0
		;

		*y = current_point.point.y * ratio_0_3
			+ 3 * data[1].point.y * ratio_1_2
			+ 3 * data[2].point.y * ratio_2_1
			+     data[3].point.y * ratio_3_0
		;

		/* Bezier gradient */
		dx = -3 * current_point.point.x * ratio_0_2
			+ 3 * data[1].point.x * _1__4ratio_1_0_3ratio_2_0
			+ 3 * data[2].point.x * _2ratio_1_0_3ratio_2_0
			+ 3 * data[3].point.x * ratio_2_0
		;

		dy =-3 * current_point.point.y * ratio_0_2
			+ 3 * data[1].point.y * _1__4ratio_1_0_3ratio_2_0
			+ 3 * data[2].point.y * _2ratio_1_0_3ratio_2_0
			+ 3 * data[3].point.y * ratio_2_0
		;

		ratio  = the_y / sqrt(dx * dx + dy * dy);
		*x    += -dy * ratio;
		*y    += dx * ratio;

		break;
	}
	}
}

inline cairo_bool_t map_path_onto(cairo_t* cr, cairo_path_t* path) {
	cairo_path_t* cur_path = 0;

	parametrized_path_t param;

	if(cairo_status(cr) || !path) return FALSE;

	cur_path = cairo_copy_path(cr);

	param.path = path;
	param.parametrization = parametrize_path(path);

	cairo_new_path(cr);

	transform_path(
		cur_path,
		(transform_point_func_t)(point_on_path),
		&param
	);

	cairo_append_path(cr, cur_path);

	free(param.parametrization);

	cairo_path_destroy(cur_path);

	return TRUE;
}

#endif
