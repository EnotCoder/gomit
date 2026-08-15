/**************************************************************************/
/*  scatter_painter_editor_plugin.h                                       */
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

#include "editor/plugins/editor_plugin.h"
#include "scene/gui/control.h"

class Button;
class Camera3D;
class CheckBox;
class EditorDock;
class EditorResourcePicker;
class Label;
class ScatterPainter3D;
class ScatterPainterEditorPlugin;
class SpinBox;

class ScatterPainterEditor : public Control {
	GDCLASS(ScatterPainterEditor, Control);

	ScatterPainter3D *node = nullptr;
	ScatterPainterEditorPlugin *plugin = nullptr;

	EditorResourcePicker *mesh_picker = nullptr;
	EditorResourcePicker *scene_picker = nullptr;
	SpinBox *brush_radius_spin = nullptr;
	SpinBox *spacing_spin = nullptr;
	SpinBox *scale_min_spin = nullptr;
	SpinBox *scale_max_spin = nullptr;
	SpinBox *rotation_spin = nullptr;
	SpinBox *tilt_spin = nullptr;
	SpinBox *seed_spin = nullptr;
	CheckBox *align_check = nullptr;
	Button *randomize_seed_button = nullptr;
	Button *clear_button = nullptr;
	Label *stats_label = nullptr;

	Button *paint_button = nullptr;
	Button *erase_button = nullptr;

	bool paint_mode = true;
	bool painting = false;
	bool erasing = false;
	bool stroke_active = false;
	Array stroke_before;

	Vector3 last_paint_point;
	bool has_last_paint_point = false;

	Camera3D *last_camera = nullptr;
	Vector3 overlay_hit;
	bool has_overlay_hit = false;

	bool _raycast(Camera3D *p_camera, const Vector2 &p_pos, Vector3 &r_position, Vector3 &r_normal);
	void _gather_collision_rids(Node *p_node, HashSet<RID> &r_rids) const;
	void _apply_stroke_at(Camera3D *p_camera, const Vector2 &p_pos);
	void _commit_stroke();
	void _update_stats();
	void _queue_overlay_redraw();

	void _set_tool(bool p_paint);
	void _brush_setting_changed();
	void _mesh_picker_changed(const Ref<Resource> &p_resource);
	void _scene_picker_changed(const Ref<Resource> &p_resource);
	void _randomize_seed();
	void _clear_all();
	void _confirm_clear();

protected:
	static void _bind_methods();

public:
	void edit(ScatterPainter3D *p_node);
	void update_brush_settings();
	void set_plugin(ScatterPainterEditorPlugin *p_plugin) { plugin = p_plugin; }

	virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event);
	virtual void forward_3d_draw_over_viewport(Control *p_overlay);

	ScatterPainterEditor();
};

class ScatterPainterEditorPlugin : public EditorPlugin {
	GDCLASS(ScatterPainterEditorPlugin, EditorPlugin);

	ScatterPainterEditor *editor = nullptr;
	EditorDock *painter_dock = nullptr;

public:
	virtual String get_plugin_name() const override { return "ScatterPainter3DEditor"; }

	virtual void edit(Object *p_object) override;
	virtual bool handles(Object *p_object) const override;
	virtual void make_visible(bool p_visible) override;

	virtual EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;
	virtual void forward_3d_draw_over_viewport(Control *p_overlay) override;

protected:
	void _notification(int p_what);

public:
	ScatterPainterEditorPlugin();
};