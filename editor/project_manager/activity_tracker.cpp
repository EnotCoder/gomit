/**************************************************************************/
/*  activity_tracker.cpp                                                  */
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

#include "activity_tracker.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "editor/file_system/editor_paths.h"

String ActivityTracker::get_log_path() {
	if (!EditorPaths::get_singleton()) {
		return String();
	}
	return EditorPaths::get_singleton()->get_config_dir().path_join("gomit_activity.log");
}

static void _append_line(const String &p_path, const String &p_line) {
	if (p_path.is_empty()) {
		return;
	}
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ_WRITE); // Append (doesn't truncate).
	if (f.is_null()) {
		// File doesn't exist yet, create it.
		f = FileAccess::open(p_path, FileAccess::WRITE);
		if (f.is_null()) {
			return;
		}
	} else {
		f->seek_end();
	}
	f->store_line(p_line);
}

void ActivityTracker::begin_session() {
	_append_line(get_log_path(), "B " + itos(Time::get_singleton()->get_unix_time_from_system()));
}

void ActivityTracker::end_session() {
	_append_line(get_log_path(), "E " + itos(Time::get_singleton()->get_unix_time_from_system()));
}

HashMap<int64_t, int64_t> ActivityTracker::get_daily_minutes() {
	HashMap<int64_t, int64_t> result;
	const String path = get_log_path();
	if (path.is_empty() || !FileAccess::exists(path)) {
		return result;
	}

	Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
	if (f.is_null()) {
		return result;
	}

	Time *time = Time::get_singleton();

	auto add_session = [&](int64_t p_start, int64_t p_end) {
		if (p_end <= p_start) {
			return;
		}
		int64_t cur = p_start;
		while (cur < p_end) {
			// Local midnight of the current day.
			const Dictionary date = time->get_date_dict_from_unix_time(cur);
			const int64_t day_start = time->get_unix_time_from_datetime_dict(date);
			// Snap to the local midnight of the following day (handles DST).
			const int64_t next_day_start = time->get_unix_time_from_datetime_dict(time->get_date_dict_from_unix_time(day_start + 86400));
			const int64_t seg_end = MIN(p_end, next_day_start);
			result[day_start] = result.has(day_start) ? result[day_start] + (seg_end - cur) : (seg_end - cur);
			cur = next_day_start;
		}
	};

	int64_t session_start = -1;
	while (!f->eof_reached()) {
		const String line = f->get_line().strip_edges();
		if (line.is_empty()) {
			continue;
		}
		const PackedStringArray parts = line.split(" ");
		if (parts.size() != 2) {
			continue;
		}
		const int64_t t = parts[1].to_int();
		if (parts[0] == "B") {
			// A previous unclosed session (crash) is discarded.
			session_start = t;
		} else if (parts[0] == "E") {
			if (session_start >= 0) {
				add_session(session_start, t);
				session_start = -1;
			}
		}
	}
	// Any unclosed session (crash or currently running editor) is discarded.

	// Convert seconds to whole minutes.
	for (KeyValue<int64_t, int64_t> &E : result) {
		E.value = E.value / 60;
	}
	return result;
}
