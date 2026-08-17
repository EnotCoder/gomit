/**************************************************************************/
/*  activity_graph.cpp                                                    */
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

#include "activity_graph.h"

#include "core/os/time.h"
#include "editor/themes/editor_scale.h"
#include "scene/scene_string_names.h"

namespace {
constexpr int ACTIVITY_LEVELS = 5;

const Color LEVEL_COLORS[ACTIVITY_LEVELS] = {
	Color(0.13f, 0.15f, 0.18f),
	Color(0.15f, 0.45f, 0.28f),
	Color(0.13f, 0.57f, 0.32f),
	Color(0.11f, 0.70f, 0.36f),
	Color(0.24f, 0.86f, 0.47f),
};

const char *MONTH_NAMES[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char *DAY_NAMES[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

// Shared layout metrics (GitHub style).
constexpr float CELL = 10.0f;
constexpr float GAP = 3.0f;
constexpr float EDGE_MARGIN = 16.0f;
constexpr float DAY_LABEL_MARGIN = 8.0f;
constexpr float TOP_STRIP_EXTRA = 8.0f;
} // namespace

void ActivityGraph::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		_draw_graph();
	}
}

int ActivityGraph::level_for_minutes(int64_t p_minutes) const {
	if (p_minutes <= 0) {
		return 0;
	}
	if (p_minutes < 30) {
		return 1;
	}
	if (p_minutes < 120) {
		return 2;
	}
	if (p_minutes < 300) {
		return 3;
	}
	return 4;
}

void ActivityGraph::set_data(const HashMap<int64_t, int64_t> &p_minutes) {
	daily_minutes = p_minutes;
	has_data = true;
	_recompute_grid();
	queue_redraw();
	update_minimum_size();
}

void ActivityGraph::_recompute_grid() {
	Time *time = Time::get_singleton();
	const int64_t now = time->get_unix_time_from_system();
	today_start = time->get_unix_time_from_datetime_dict(time->get_date_dict_from_unix_time(now));

	// ~365 days back, snapped to the previous Sunday so the grid lines up with weeks.
	const int64_t start_ts = time->get_unix_time_from_datetime_dict(time->get_date_dict_from_unix_time(today_start - 364 * 86400));
	const Dictionary start_date = time->get_date_dict_from_unix_time(start_ts);
	const int weekday = start_date["weekday"]; // weekday 0 = Sunday.
	grid_start = time->get_unix_time_from_datetime_dict(time->get_date_dict_from_unix_time(start_ts - weekday * 86400));

	const int64_t day_count = (today_start - grid_start) / 86400 + 1;
	week_count = (day_count - 1) / 7 + 1;
}

void ActivityGraph::_draw_graph() {
	if (!has_data) {
		return;
	}

	Time *time = Time::get_singleton();
	Ref<Font> font = get_theme_font(SceneStringName(font));
	const int font_size = get_theme_font_size(SceneStringName(font_size));
	if (font.is_null()) {
		return;
	}

	const float cell = CELL * EDSCALE;
	const float gap = GAP * EDSCALE;
	const float step = cell + gap;
	const float ascent = font->get_ascent(font_size);

	const float day_label_w = font->get_string_size("Wed", HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x + DAY_LABEL_MARGIN * EDSCALE;
	const float top_strip = ascent + TOP_STRIP_EXTRA * EDSCALE;

	const Size2 size = get_size();
	const float grid_width = week_count * step;
	const float block_width = day_label_w + grid_width;
	const float block_height = top_strip + 7 * step + cell + 20.0f * EDSCALE;

	// Center the block (day labels + grid) in the available space. Re-computed
	// on every draw, so it always matches the current window size.
	const float offset_x = MAX(0.0f, (size.width - block_width) * 0.5f);
	const float offset_y = MAX(0.0f, (size.height - block_height) * 0.5f);
	const float grid_x = offset_x + day_label_w;
	const float grid_y = offset_y + top_strip;

	// Month labels, slightly above the grid so they don't touch the cells.
	{
		int64_t week_start = grid_start;
		int prev_month = -1;
		for (int col = 0; col < week_count; col++) {
			const Dictionary d = time->get_date_dict_from_unix_time(week_start);
			const int month = d["month"];
			if (month != prev_month) {
				prev_month = month;
				const String label = MONTH_NAMES[month - 1];
				const float tw = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
				draw_string(font, Point2(grid_x + col * step + (step - tw) * 0.5f, grid_y - 4.0f * EDSCALE), label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.75f, 0.75f, 0.75f));
			}
			week_start = time->get_unix_time_from_datetime_dict(time->get_date_dict_from_unix_time(week_start + 7 * 86400));
		}
	}

	// Day-of-week labels on the left (Mon, Wed, Fri), centered on their rows.
	{
		for (int row = 1; row < 7; row += 2) {
			const String label = DAY_NAMES[row];
			const float w = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
			const float baseline = grid_y + row * step + cell * 0.5f + ascent * 0.5f;
			draw_string(font, Point2(grid_x - 4.0f * EDSCALE - w, baseline), label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.6f, 0.6f, 0.6f));
		}
	}

	// Heatmap cells.
	{
		int64_t cur = grid_start;
		for (int col = 0; col < week_count && cur <= today_start; col++) {
			for (int row = 0; row < 7; row++) {
				if (cur > today_start) {
					break;
				}
				const int64_t minutes = daily_minutes.has(cur) ? daily_minutes[cur] : 0;
				const int level = level_for_minutes(minutes);
				Color color = LEVEL_COLORS[level];
				if (level == 0) {
					// Slightly translucent backdrop so it blends with any theme.
					color = Color(0.35f, 0.37f, 0.40f, 0.25f);
				}
				draw_rect(Rect2(grid_x + col * step, grid_y + row * step, cell, cell), color);
				cur = time->get_unix_time_from_datetime_dict(time->get_date_dict_from_unix_time(cur + 86400));
			}
		}
	}

	// Legend centered below the grid (Less ... More).
	{
		const String less = TTRC("Less");
		const String more = TTRC("More");
		const float less_w = font->get_string_size(less, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
		const float more_w = font->get_string_size(more, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
		const float pad = 4.0f * EDSCALE;
		const float legend_start = grid_x + grid_width * 0.5f - (less_w + pad * 3.0f + ACTIVITY_LEVELS * cell + (ACTIVITY_LEVELS - 1) * gap + more_w) * 0.5f;
		const float baseline = offset_y + block_height - 8.0f * EDSCALE;

		float x = legend_start;
		draw_string(font, Point2(x, baseline), less, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.6f, 0.6f, 0.6f));
		x += less_w + pad;
		for (int level = 0; level < ACTIVITY_LEVELS; level++) {
			Color color = LEVEL_COLORS[level];
			if (level == 0) {
				color = Color(0.35f, 0.37f, 0.40f, 0.25f);
			}
			draw_rect(Rect2(x, baseline - cell, cell, cell), color);
			x += cell + gap;
		}
		x += pad - gap;
		draw_string(font, Point2(x, baseline), more, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0.6f, 0.6f, 0.6f));
	}
}

Size2 ActivityGraph::get_minimum_size() const {
	if (!has_data || week_count <= 0) {
		return Size2(400.0f * EDSCALE, 120.0f * EDSCALE);
	}
	Ref<Font> font = get_theme_font(SceneStringName(font));
	const int font_size = get_theme_font_size(SceneStringName(font_size));
	if (font.is_null()) {
		return Size2(400.0f * EDSCALE, 120.0f * EDSCALE);
	}
	const float cell = CELL * EDSCALE;
	const float gap = GAP * EDSCALE;
	const float step = cell + gap;
	const float day_label_w = font->get_string_size("Wed", HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x + DAY_LABEL_MARGIN * EDSCALE;
	const float top_strip = font->get_ascent(font_size) + TOP_STRIP_EXTRA * EDSCALE;
	return Size2(day_label_w + week_count * step + 2.0f * EDGE_MARGIN * EDSCALE, top_strip + 7 * step + cell + 24.0f * EDSCALE);
}

int64_t ActivityGraph::day_of_cell_at(const Vector2 &p_pos) const {
	Time *time = Time::get_singleton();
	Ref<Font> font = get_theme_font(SceneStringName(font));
	const int font_size = get_theme_font_size(SceneStringName(font_size));
	if (font.is_null()) {
		return -1;
	}

	const float cell = CELL * EDSCALE;
	const float gap = GAP * EDSCALE;
	const float step = cell + gap;
	const float day_label_w = font->get_string_size("Wed", HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x + DAY_LABEL_MARGIN * EDSCALE;
	const float top_strip = font->get_ascent(font_size) + TOP_STRIP_EXTRA * EDSCALE;

	const Size2 size = get_size();
	const float grid_width = week_count * step;
	const float block_width = day_label_w + grid_width;
	const float block_height = top_strip + 7 * step + cell + 20.0f * EDSCALE;
	const float offset_x = MAX(0.0f, (size.width - block_width) * 0.5f);
	const float offset_y = MAX(0.0f, (size.height - block_height) * 0.5f);
	const float grid_x = offset_x + day_label_w;
	const float grid_y = offset_y + top_strip;

	const int col = (int)((p_pos.x - grid_x) / step);
	const int row = (int)((p_pos.y - grid_y) / step);
	if (col < 0 || row < 0 || col >= week_count || row >= 7) {
		return -1;
	}
	const float cell_x = grid_x + col * step;
	const float cell_y = grid_y + row * step;
	if (p_pos.x < cell_x || p_pos.x > cell_x + cell || p_pos.y < cell_y || p_pos.y > cell_y + cell) {
		return -1;
	}

	int64_t cur = grid_start;
	for (int c = 0; c < col; c++) {
		cur = time->get_unix_time_from_datetime_dict(time->get_date_dict_from_unix_time(cur + 7 * 86400));
	}
	for (int r = 0; r < row; r++) {
		cur = time->get_unix_time_from_datetime_dict(time->get_date_dict_from_unix_time(cur + 86400));
	}
	return cur;
}

String ActivityGraph::get_tooltip(const Point2 &p_pos) const {
	ActivityGraph *self = const_cast<ActivityGraph *>(this);
	const int64_t cur = self->day_of_cell_at(p_pos);
	if (cur < 0 || cur > today_start) {
		return String();
	}
	Time *time = Time::get_singleton();
	const int64_t minutes = daily_minutes.has(cur) ? daily_minutes[cur] : 0;
	const String date_str = time->get_date_string_from_unix_time(cur);
	if (minutes <= 0) {
		return vformat("%s\n%s", date_str, TTR("No activity"));
	}
	return vformat("%s\n%s", date_str, vformat(TTRN("%d minute of work", "%d minutes of work", minutes), minutes));
}

ActivityGraph::ActivityGraph() {
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_tooltip_text(""); // Enable tooltips during hover.
}