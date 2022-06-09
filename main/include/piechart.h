#pragma once

#include <pax_gfx.h>
#include <stddef.h>

typedef struct {
	// The name to show next to the slice, if any.
	const char *name;
	// The part of the total that this slice represents.
	float       part;
	// The color of this slice.
	pax_col_t   color;
} piechart_slice_t;

// Draws a pie chart, X/Y are center.
void piechart(pax_buf_t *buf, float x, float y, float radius, const piechart_slice_t *slices, size_t n_slices);
