/**************************************************************************/
/*  scatter_painter_editor_plugin.cpp                                     */
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

#include "scatter_painter_editor_plugin.h"

#include "core/input/input_enums.h"
#include "core/object/callable_mp.h"
#include "core/string/ustring.h"
#include "editor/docks/editor_dock.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_resource_picker.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/3d/scatter_painter_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/base_button.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/3d/world_3d.h"
#include "servers/physics_3d/direct_states/physics_direct_space_state_3d.h"
#include "servers/physics_3d/physics_server_3d_types.h"
#include "servers/text/text_server.h"

void ScatterPainterEditor::edit(ScatterPainter3D *p_node) {
	node = p_node;
	_sync_tool_buttons();
	update_brush_settings();
}

void ScatterPainterEditor::update_brush_settings() {
	if (node) {
		brush_radius_spin->set_value(node->get_brush_radius());
		spacing_spin->set_value(node->get_spacing());
		scale_min_spin->set_value(node->get_scale_min());
		scale_max_spin->set_value(node->get_scale_max());
		rotation_spin->set_value(node->get_random_rotation_degrees());
		tilt_spin->set_value(node->get_random_tilt_degrees());
		seed_spin->set_value(node->get_random_seed());
		align_check->set_pressed(node->is_align_to_surface());
		mesh_picker->set_edited_resource(node->get_painting_mesh());
		scene_picker->set_edited_resource(node->get_painting_scene());
	} else {
		mesh_picker->set_edited_resource(Ref<Resource>());
		scene_picker->set_edited_resource(Ref<Resource>());
	}
	update_stats();
}

void ScatterPainterEditor::update_stats() {
	if (!stats_label) {
		return;
	}
	if (node) {
		stats_label->set_text(vformat(TTR("Instances: %d"), node->get_instance_count()));
	} else {
		stats_label->set_text(TTR("No ScatterPainter3D selected."));
	}
}

void ScatterPainterEditor::_set_tool(bool p_paint) {
	if (plugin) {
		plugin->set_paint_mode(p_paint);
		_sync_tool_buttons();
	}
}

void ScatterPainterEditor::_sync_tool_buttons() {
	if (!paint_button || !erase_button) {
		return;
	}
	const bool paint_active = plugin ? plugin->is_paint_mode() : true;
	paint_button->set_pressed(paint_active);
	erase_button->set_pressed(!paint_active);
}

void ScatterPainterEditor::_brush_setting_changed() {
	if (!node) {
		return;
	}
	node->set_brush_radius(brush_radius_spin->get_value());
	node->set_spacing(spacing_spin->get_value());
	node->set_scale_min(scale_min_spin->get_value());
	node->set_scale_max(scale_max_spin->get_value());
	node->set_random_rotation_degrees(rotation_spin->get_value());
	node->set_random_tilt_degrees(tilt_spin->get_value());
	node->set_random_seed(seed_spin->get_value());
	node->set_align_to_surface(align_check->is_pressed());
}

void ScatterPainterEditor::_mesh_picker_changed(const Ref<Resource> &p_resource) {
	if (!node) {
		return;
	}
	node->set_painting_mesh(p_resource);
}

void ScatterPainterEditor::_scene_picker_changed(const Ref<Resource> &p_resource) {
	if (!node) {
		return;
	}
	node->set_painting_scene(p_resource);
}

void ScatterPainterEditor::_randomize_seed() {
	seed_spin->set_value((int)Math::rand());
	_brush_setting_changed();
}

void ScatterPainterEditor::_clear_all() {
	if (!node || node->get_instance_count() == 0) {
		return;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Clear Scatter Instances"), UndoRedo::MERGE_DISABLE, node);
	undo_redo->add_do_method(node, SNAME("clear_instances"));
	undo_redo->add_undo_method(node, SNAME("set_instances"), node->get_instances());
	undo_redo->commit_action();
	update_stats();
}

void ScatterPainterEditor::_confirm_clear() {
	_clear_all();
}

void ScatterPainterEditor::_bind_methods() {
}

ScatterPainterEditor::ScatterPainterEditor() {
	set_name("ScatterPainterEditor");
	set_h_size_flags(SIZE_EXPAND_FILL);

	VBoxContainer *main = memnew(VBoxContainer);
	main->set_anchors_preset(Control::PRESET_FULL_RECT);
	add_child(main);

	HBoxContainer *toolbar = memnew(HBoxContainer);
	main->add_child(toolbar);

	paint_button = memnew(Button);
	paint_button->set_toggle_mode(true);
	paint_button->set_text(TTR("Paint"));
	erase_button = memnew(Button);
	erase_button->set_toggle_mode(true);
	erase_button->set_text(TTR("Erase"));

	Ref<ButtonGroup> button_group;
	button_group.instantiate();
	paint_button->set_button_group(button_group);
	erase_button->set_button_group(button_group);
	_sync_tool_buttons();

	paint_button->connect(SceneStringName(pressed), callable_mp(this, &ScatterPainterEditor::_set_tool).bind(true));
	erase_button->connect(SceneStringName(pressed), callable_mp(this, &ScatterPainterEditor::_set_tool).bind(false));

	toolbar->add_child(paint_button);
	toolbar->add_child(erase_button);

	main->add_child(memnew(HSeparator));

	Label *source_label = memnew(Label);
	source_label->set_text(TTR("Source"));
	source_label->set_theme_type_variation("HeaderSmall");
	main->add_child(source_label);

	mesh_picker = memnew(EditorResourcePicker);
	mesh_picker->set_base_type("Mesh");
	mesh_picker->set_h_size_flags(SIZE_EXPAND_FILL);
	mesh_picker->connect(SNAME("resource_changed"), callable_mp(this, &ScatterPainterEditor::_mesh_picker_changed));
	main->add_margin_child(TTR("Mesh:"), mesh_picker);

	scene_picker = memnew(EditorResourcePicker);
	scene_picker->set_base_type("PackedScene");
	scene_picker->set_h_size_flags(SIZE_EXPAND_FILL);
	scene_picker->connect(SNAME("resource_changed"), callable_mp(this, &ScatterPainterEditor::_scene_picker_changed));
	main->add_margin_child(TTR("Scene:"), scene_picker);

	main->add_child(memnew(HSeparator));

	Label *brush_label = memnew(Label);
	brush_label->set_text(TTR("Brush"));
	brush_label->set_theme_type_variation("HeaderSmall");
	main->add_child(brush_label);

	Callable brush_changed = callable_mp(this, &ScatterPainterEditor::_brush_setting_changed);

	auto create_spin = [&](double p_min, double p_max, double p_step, double p_value) {
		SpinBox *spin = memnew(SpinBox);
		spin->set_min(p_min);
		spin->set_max(p_max);
		spin->set_step(p_step);
		spin->set_value(p_value);
		spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		spin->connect(SceneStringName(value_changed), brush_changed);
		return spin;
	};

	brush_radius_spin = create_spin(0.1, 500.0, 0.1, 5.0);
	main->add_margin_child(TTR("Brush Radius:"), brush_radius_spin);

	spacing_spin = create_spin(0.01, 100.0, 0.01, 1.0);
	main->add_margin_child(TTR("Spacing:"), spacing_spin);

	scale_min_spin = create_spin(0.001, 1000.0, 0.001, 0.8);
	main->add_margin_child(TTR("Scale Min:"), scale_min_spin);

	scale_max_spin = create_spin(0.001, 1000.0, 0.001, 1.2);
	main->add_margin_child(TTR("Scale Max:"), scale_max_spin);

	rotation_spin = create_spin(0.0, 360.0, 0.1, 45.0);
	main->add_margin_child(TTR("Random Rotation:"), rotation_spin);

	tilt_spin = create_spin(0.0, 90.0, 0.1, 10.0);
	main->add_margin_child(TTR("Random Tilt:"), tilt_spin);

	align_check = memnew(CheckBox);
	align_check->set_text(TTR("Align to Surface"));
	align_check->set_pressed(true);
	align_check->connect(SceneStringName(toggled), brush_changed);
	main->add_child(align_check);

	HBoxContainer *seed_box = memnew(HBoxContainer);
	seed_box->set_h_size_flags(SIZE_EXPAND_FILL);
	seed_spin = create_spin(0, 2147483647, 1, 0);
	main->add_margin_child(TTR("Random Seed:"), seed_box);

	randomize_seed_button = memnew(Button);
	randomize_seed_button->set_text(TTR("Randomize"));
	seed_box->add_child(seed_spin);
	seed_box->add_child(randomize_seed_button);
	randomize_seed_button->connect(SceneStringName(pressed), callable_mp(this, &ScatterPainterEditor::_randomize_seed));

	main->add_child(memnew(HSeparator));

	clear_button = memnew(Button);
	clear_button->set_text(TTR("Clear All"));
	clear_button->connect(SceneStringName(pressed), callable_mp(this, &ScatterPainterEditor::_clear_all));
	main->add_child(clear_button);

	stats_label = memnew(Label);
	stats_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	main->add_child(stats_label);

	update_stats();
}

///////////////////////

void ScatterPainterEditorPlugin::_reset_stroke_state() {
	painting = false;
	erasing = false;
	stroke_active = false;
	has_last_paint_point = false;
	has_overlay_hit = false;
}

void ScatterPainterEditorPlugin::edit(Object *p_object) {
	ScatterPainter3D *new_node = Object::cast_to<ScatterPainter3D>(p_object);
	if (new_node == edited_node) {
		return;
	}
	edited_node = new_node;
	_reset_stroke_state();
	if (painter_editor) {
		painter_editor->edit(edited_node);
	}
}

bool ScatterPainterEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<ScatterPainter3D>(p_object) != nullptr;
}

void ScatterPainterEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		if (painter_dock) {
			painter_dock->open();
		}
	} else {
		if (painter_dock) {
			painter_dock->close();
		}
		edited_node = nullptr;
		_reset_stroke_state();
		if (painter_editor) {
			painter_editor->edit(nullptr);
		}
	}
}

void ScatterPainterEditorPlugin::set_paint_mode(bool p_paint) {
	if (paint_mode == p_paint) {
		return;
	}
	paint_mode = p_paint;
	_reset_stroke_state();
	update_overlays();
}

void ScatterPainterEditorPlugin::_gather_collision_rids(Node *p_node, HashSet<RID> &r_rids) const {
	if (!p_node) {
		return;
	}
	CollisionObject3D *collision_object = Object::cast_to<CollisionObject3D>(p_node);
	if (collision_object) {
		r_rids.insert(collision_object->get_rid());
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_gather_collision_rids(p_node->get_child(i), r_rids);
	}
}

bool ScatterPainterEditorPlugin::_raycast(Camera3D *p_camera, const Vector2 &p_pos, Vector3 &r_position, Vector3 &r_normal) {
	ERR_FAIL_NULL_V(edited_node, false);
	if (!edited_node->is_inside_tree()) {
		return false;
	}

	Ref<World3D> world = edited_node->get_world_3d();
	ERR_FAIL_NULL_V(world, false);

	PhysicsDirectSpaceState3D *space = world->get_direct_space_state();
	ERR_FAIL_NULL_V(space, false);

	PS3DT::RayParameters params;
	params.from = p_camera->project_ray_origin(p_pos);
	params.to = params.from + p_camera->project_ray_normal(p_pos) * p_camera->get_far();
	_gather_collision_rids(edited_node, params.exclude);

	PS3DT::RayResult result;
	if (!space->intersect_ray(params, result)) {
		return false;
	}

	r_position = result.position;
	r_normal = result.normal;
	return true;
}

void ScatterPainterEditorPlugin::_apply_stroke_at(Camera3D *p_camera, const Vector2 &p_pos) {
	Vector3 hit;
	Vector3 normal;
	if (!_raycast(p_camera, p_pos, hit, normal)) {
		has_overlay_hit = false;
		update_overlays();
		return;
	}

	has_overlay_hit = true;
	overlay_hit = hit;
	last_paint_point = hit;
	has_last_paint_point = true;

	if (painting) {
		edited_node->paint_stroke(hit, normal);
	} else if (erasing) {
		edited_node->erase_near(hit, edited_node->get_brush_radius());
	}
	if (painter_editor) {
		painter_editor->update_stats();
	}
	update_overlays();
}

void ScatterPainterEditorPlugin::_commit_stroke() {
	if (!edited_node || !stroke_active) {
		return;
	}
	stroke_active = false;

	Array after = edited_node->get_instances();
	if (after == stroke_before) {
		return;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Paint Scatter Instances"), UndoRedo::MERGE_DISABLE, edited_node);
	undo_redo->add_do_method(edited_node, SNAME("set_instances"), after);
	undo_redo->add_undo_method(edited_node, SNAME("set_instances"), stroke_before);
	undo_redo->commit_action();
}

EditorPlugin::AfterGUIInput ScatterPainterEditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	if (!edited_node) {
		return EditorPlugin::AFTER_GUI_INPUT_PASS;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				stroke_active = true;
				stroke_before = edited_node->get_instances();
				painting = paint_mode;
				erasing = !paint_mode;
				last_camera = p_camera;
				_apply_stroke_at(p_camera, mb->get_position());
				return EditorPlugin::AFTER_GUI_INPUT_STOP;
			} else {
				_commit_stroke();
				painting = false;
				erasing = false;
				has_last_paint_point = false;
			}
		}
		return EditorPlugin::AFTER_GUI_INPUT_STOP;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		last_camera = p_camera;

		if (painting) {
			Vector3 hit;
			Vector3 normal;
			if (_raycast(p_camera, mm->get_position(), hit, normal)) {
				has_overlay_hit = true;
				overlay_hit = hit;
				const real_t step = MAX(edited_node->get_spacing(), 0.01);
				const bool moved = !has_last_paint_point || hit.distance_to(last_paint_point) >= step;
				if (moved) {
					has_last_paint_point = true;
					last_paint_point = hit;
					edited_node->paint_stroke(hit, normal);
					if (painter_editor) {
						painter_editor->update_stats();
					}
				}
			} else {
				has_overlay_hit = false;
			}
			update_overlays();
			return EditorPlugin::AFTER_GUI_INPUT_STOP;
		}

		if (erasing) {
			Vector3 hit;
			Vector3 normal;
			if (_raycast(p_camera, mm->get_position(), hit, normal)) {
				has_overlay_hit = true;
				overlay_hit = hit;
				edited_node->erase_near(hit, edited_node->get_brush_radius());
				if (painter_editor) {
					painter_editor->update_stats();
				}
			} else {
				has_overlay_hit = false;
			}
			update_overlays();
			return EditorPlugin::AFTER_GUI_INPUT_STOP;
		}

		// Idle hover: update the brush radius preview.
		Vector3 hit;
		Vector3 normal;
		if (_raycast(p_camera, mm->get_position(), hit, normal)) {
			has_overlay_hit = true;
			overlay_hit = hit;
		} else {
			has_overlay_hit = false;
		}
		update_overlays();
	}

	return EditorPlugin::AFTER_GUI_INPUT_PASS;
}

void ScatterPainterEditorPlugin::forward_3d_draw_over_viewport(Control *p_overlay) {
	if (!edited_node || !has_overlay_hit || !last_camera) {
		return;
	}

	const Vector2 center_2d = last_camera->unproject_position(overlay_hit);
	const Vector3 camera_right = last_camera->get_global_transform().basis.get_column(0);
	const Vector2 edge_2d = last_camera->unproject_position(overlay_hit + camera_right * edited_node->get_brush_radius());
	const real_t radius_2d = MAX(center_2d.distance_to(edge_2d), 1.0);

	const Color color = paint_mode ? Color(0.3, 0.9, 0.3, 0.15) : Color(0.95, 0.3, 0.3, 0.15);
	p_overlay->draw_circle(center_2d, radius_2d, color);
	p_overlay->draw_arc(center_2d, radius_2d, 0, Math::TAU, 64, color.lightened(0.3), 2.0);
}

void ScatterPainterEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_force_draw_over_forwarding_enabled();
		} break;
	}
}

///////////////////////

class ScatterPainterInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(ScatterPainterInspectorPlugin, EditorInspectorPlugin);

	bool can_handle(Object *p_object) override {
		return Object::cast_to<ScatterPainter3D>(p_object) != nullptr;
	}

	bool parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide) override {
		// These are presented (and edited) by the painter dock,
		// so hide them from the inspector and keep only the standard node properties.
		static const HashSet<String> hidden_props = {
			"painting_mesh",
			"painting_scene",
			"instances",
			"brush_radius",
			"spacing",
			"scale_min",
			"scale_max",
			"random_rotation_degrees",
			"random_tilt_degrees",
			"random_seed",
			"align_to_surface",
		};
		return hidden_props.has(p_path);
	}
};

ScatterPainterEditorPlugin::ScatterPainterEditorPlugin() {
	painter_editor = memnew(ScatterPainterEditor);
	painter_editor->set_plugin(this);
	painter_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	painter_dock = memnew(EditorDock);
	painter_dock->set_name("ScatterPainter3D");
	painter_dock->set_title(TTR("Scatter Painter 3D"));
	painter_dock->set_icon_name("ScatterPainter3D");
	painter_dock->set_default_slot(EditorDock::DOCK_SLOT_RIGHT_UR);
	painter_dock->set_available_layouts(EditorDock::DOCK_LAYOUT_ALL);
	painter_dock->set_global(false);
	painter_dock->set_transient(true);
	painter_dock->add_child(painter_editor);

	EditorDockManager::get_singleton()->add_dock(painter_dock);
	painter_dock->close();

	Ref<ScatterPainterInspectorPlugin> plugin;
	plugin.instantiate();
	add_inspector_plugin(plugin);
}