/**************************************************************************/
/*  activity_graph.h                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/templates/hash_map.h"
#include "scene/gui/control.h"

// Control that draws a GitHub-style activity heatmap.
class ActivityGraph : public Control {
	GDCLASS(ActivityGraph, Control);

	HashMap<int64_t, int64_t> daily_minutes;

	int64_t grid_start = 0; // Local-midnight unix timestamp of the first Sunday.
	int64_t today_start = 0;
	int week_count = 0;

	bool has_data = false;

	void _recompute_grid();
	void _draw_graph();
	int level_for_minutes(int64_t p_minutes) const;
	int64_t day_of_cell_at(const Vector2 &p_pos) const;

protected:
	void _notification(int p_what);
	virtual String get_tooltip(const Point2 &p_pos) const override;
	virtual Size2 get_minimum_size() const override;

public:
	void set_data(const HashMap<int64_t, int64_t> &p_minutes);

	ActivityGraph();
};