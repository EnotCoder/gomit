/**************************************************************************/
/*  scatter_painter_3d.cpp                                                */
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

#include "scatter_painter_3d.h"

#include "core/object/class_db.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/resources/multimesh.h"
#include "scene/resources/packed_scene.h"

void ScatterPainter3D::_ensure_internal_nodes() {
	if (!is_inside_tree()) {
		return;
	}

	if (multimesh_instance == nullptr) {
		multimesh_instance = memnew(MultiMeshInstance3D);
		multimesh_instance->set_name("_MultiMesh");
		add_child(multimesh_instance, true, Node::INTERNAL_MODE_BACK);
	}

	if (scene_instances_root == nullptr) {
		scene_instances_root = memnew(Node3D);
		scene_instances_root->set_name("_SceneInstances");
		add_child(scene_instances_root, true, Node::INTERNAL_MODE_BACK);
	}
}

void ScatterPainter3D::_rebuild_multimesh() {
	if (multimesh_instance == nullptr) {
		return;
	}

	Ref<MultiMesh> mm = multimesh_instance->get_multimesh();

	if (!painting_mesh.is_valid() || instances.is_empty()) {
		multimesh_instance->set_multimesh(Ref<MultiMesh>());
		return;
	}

	if (mm.is_null()) {
		mm.instantiate();
	}

	mm->set_transform_format(MultiMesh::TRANSFORM_3D);
	mm->set_mesh(painting_mesh);
	mm->set_instance_count(instances.size());
	for (int i = 0; i < instances.size(); i++) {
		mm->set_instance_transform(i, (Transform3D)(instances[i]));
	}
	multimesh_instance->set_multimesh(mm);
}

void ScatterPainter3D::_rebuild_scene_instances() {
	scene_instances_dirty = false;

	if (!is_inside_tree() || scene_instances_root == nullptr) {
		return;
	}

	for (int i = scene_instances_root->get_child_count() - 1; i >= 0; i--) {
		Node *child = scene_instances_root->get_child(i);
		scene_instances_root->remove_child(child);
		memdelete(child);
	}

	if (!painting_scene.is_valid()) {
		return;
	}

	for (int i = 0; i < instances.size(); i++) {
		Node *instance = painting_scene->instantiate();
		ERR_CONTINUE(instance == nullptr);
		scene_instances_root->add_child(instance);
		Node3D *instance_3d = Object::cast_to<Node3D>(instance);
		if (instance_3d) {
			instance_3d->set_transform((Transform3D)(instances[i]));
		}
	}
}

void ScatterPainter3D::_rebuild_scene_instances_deferred() {
	_rebuild_scene_instances();
}

Basis ScatterPainter3D::_basis_from_normal(const Vector3 &p_normal) const {
	Vector3 n = p_normal.normalized();
	if (n.length() < CMP_EPSILON) {
		n = Vector3(0, 1, 0);
	}

	Vector3 up = Vector3(0, 1, 0);
	if (Math::abs(up.dot(n)) > 0.999) {
		up = Vector3(0, 0, 1);
	}

	Vector3 x = up.cross(n);
	if (x.length() < CMP_EPSILON) {
		x = Vector3(1, 0, 0);
	} else {
		x.normalize();
	}

	Vector3 z = x.cross(n);
	z.normalize();

	return Basis(x, n, z);
}

void ScatterPainter3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_ensure_internal_nodes();
			rebuild();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			multimesh_instance = nullptr;
			scene_instances_root = nullptr;
			scene_instances_dirty = false;
		} break;
	}
}

void ScatterPainter3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_painting_mesh", "mesh"), &ScatterPainter3D::set_painting_mesh);
	ClassDB::bind_method(D_METHOD("get_painting_mesh"), &ScatterPainter3D::get_painting_mesh);
	ClassDB::bind_method(D_METHOD("set_painting_scene", "scene"), &ScatterPainter3D::set_painting_scene);
	ClassDB::bind_method(D_METHOD("get_painting_scene"), &ScatterPainter3D::get_painting_scene);
	ClassDB::bind_method(D_METHOD("set_instances", "instances"), &ScatterPainter3D::set_instances);
	ClassDB::bind_method(D_METHOD("get_instances"), &ScatterPainter3D::get_instances);
	ClassDB::bind_method(D_METHOD("set_brush_radius", "radius"), &ScatterPainter3D::set_brush_radius);
	ClassDB::bind_method(D_METHOD("get_brush_radius"), &ScatterPainter3D::get_brush_radius);
	ClassDB::bind_method(D_METHOD("set_spacing", "spacing"), &ScatterPainter3D::set_spacing);
	ClassDB::bind_method(D_METHOD("get_spacing"), &ScatterPainter3D::get_spacing);
	ClassDB::bind_method(D_METHOD("set_scale_min", "scale"), &ScatterPainter3D::set_scale_min);
	ClassDB::bind_method(D_METHOD("get_scale_min"), &ScatterPainter3D::get_scale_min);
	ClassDB::bind_method(D_METHOD("set_scale_max", "scale"), &ScatterPainter3D::set_scale_max);
	ClassDB::bind_method(D_METHOD("get_scale_max"), &ScatterPainter3D::get_scale_max);
	ClassDB::bind_method(D_METHOD("set_random_rotation_degrees", "degrees"), &ScatterPainter3D::set_random_rotation_degrees);
	ClassDB::bind_method(D_METHOD("get_random_rotation_degrees"), &ScatterPainter3D::get_random_rotation_degrees);
	ClassDB::bind_method(D_METHOD("set_random_tilt_degrees", "degrees"), &ScatterPainter3D::set_random_tilt_degrees);
	ClassDB::bind_method(D_METHOD("get_random_tilt_degrees"), &ScatterPainter3D::get_random_tilt_degrees);
	ClassDB::bind_method(D_METHOD("set_align_to_surface", "align"), &ScatterPainter3D::set_align_to_surface);
	ClassDB::bind_method(D_METHOD("is_align_to_surface"), &ScatterPainter3D::is_align_to_surface);
	ClassDB::bind_method(D_METHOD("set_random_seed", "seed"), &ScatterPainter3D::set_random_seed);
	ClassDB::bind_method(D_METHOD("get_random_seed"), &ScatterPainter3D::get_random_seed);
	ClassDB::bind_method(D_METHOD("get_instance_count"), &ScatterPainter3D::get_instance_count);
	ClassDB::bind_method(D_METHOD("paint_stroke", "center", "normal"), &ScatterPainter3D::paint_stroke);
	ClassDB::bind_method(D_METHOD("erase_near", "center", "radius"), &ScatterPainter3D::erase_near);
	ClassDB::bind_method(D_METHOD("clear_instances"), &ScatterPainter3D::clear_instances);
	ClassDB::bind_method(D_METHOD("rebuild"), &ScatterPainter3D::rebuild);
	ClassDB::bind_method(D_METHOD("_rebuild_scene_instances_deferred"), &ScatterPainter3D::_rebuild_scene_instances_deferred);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "painting_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_painting_mesh", "get_painting_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "painting_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_painting_scene", "get_painting_scene");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "instances", PROPERTY_HINT_ARRAY_TYPE, "Transform3D"), "set_instances", "get_instances");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "brush_radius", PROPERTY_HINT_RANGE, "0.1,500.0,0.1,or_less,or_greater"), "set_brush_radius", "get_brush_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spacing", PROPERTY_HINT_RANGE, "0.01,100.0,0.01,or_less,or_greater"), "set_spacing", "get_spacing");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scale_min", PROPERTY_HINT_RANGE, "0.001,1000.0,0.001,or_less,or_greater"), "set_scale_min", "get_scale_min");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scale_max", PROPERTY_HINT_RANGE, "0.001,1000.0,0.001,or_less,or_greater"), "set_scale_max", "get_scale_max");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_rotation_degrees", PROPERTY_HINT_RANGE, "0,360.0,0.1"), "set_random_rotation_degrees", "get_random_rotation_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_tilt_degrees", PROPERTY_HINT_RANGE, "0,90.0,0.1"), "set_random_tilt_degrees", "get_random_tilt_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "align_to_surface"), "set_align_to_surface", "is_align_to_surface");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_seed", PROPERTY_HINT_RANGE, "0,2147483647,1"), "set_random_seed", "get_random_seed");
}

void ScatterPainter3D::set_painting_mesh(const Ref<Mesh> &p_mesh) {
	painting_mesh = p_mesh;
	rebuild();
}

Ref<Mesh> ScatterPainter3D::get_painting_mesh() const {
	return painting_mesh;
}

void ScatterPainter3D::set_painting_scene(const Ref<PackedScene> &p_scene) {
	painting_scene = p_scene;
	rebuild();
}

Ref<PackedScene> ScatterPainter3D::get_painting_scene() const {
	return painting_scene;
}

void ScatterPainter3D::set_instances(const Array &p_instances) {
	Array filtered;
	for (int i = 0; i < p_instances.size(); i++) {
		if (p_instances[i].get_type() == Variant::TRANSFORM3D) {
			filtered.append(p_instances[i]);
		}
	}
	instances = filtered;
	notify_property_list_changed();
	rebuild();
}

Array ScatterPainter3D::get_instances() const {
	return instances;
}

void ScatterPainter3D::set_brush_radius(float p_radius) {
	brush_radius = MAX(p_radius, 0.0);
}

float ScatterPainter3D::get_brush_radius() const {
	return brush_radius;
}

void ScatterPainter3D::set_spacing(float p_spacing) {
	spacing = MAX(p_spacing, 0.001);
}

float ScatterPainter3D::get_spacing() const {
	return spacing;
}

void ScatterPainter3D::set_scale_min(float p_scale) {
	scale_min = MAX(p_scale, 0.001);
	if (scale_min > scale_max) {
		scale_min = scale_max;
	}
}

float ScatterPainter3D::get_scale_min() const {
	return scale_min;
}

void ScatterPainter3D::set_scale_max(float p_scale) {
	scale_max = MAX(p_scale, 0.001);
	if (scale_max < scale_min) {
		scale_max = scale_min;
	}
}

float ScatterPainter3D::get_scale_max() const {
	return scale_max;
}

void ScatterPainter3D::set_random_rotation_degrees(float p_degrees) {
	random_rotation_degrees = CLAMP(p_degrees, 0.0, 360.0);
}

float ScatterPainter3D::get_random_rotation_degrees() const {
	return random_rotation_degrees;
}

void ScatterPainter3D::set_random_tilt_degrees(float p_degrees) {
	random_tilt_degrees = CLAMP(p_degrees, 0.0, 90.0);
}

float ScatterPainter3D::get_random_tilt_degrees() const {
	return random_tilt_degrees;
}

void ScatterPainter3D::set_align_to_surface(bool p_align) {
	align_to_surface = p_align;
}

bool ScatterPainter3D::is_align_to_surface() const {
	return align_to_surface;
}

void ScatterPainter3D::set_random_seed(int p_seed) {
	random_seed = p_seed;
}

int ScatterPainter3D::get_random_seed() const {
	return random_seed;
}

int ScatterPainter3D::get_instance_count() const {
	return instances.size();
}

int ScatterPainter3D::paint_stroke(const Vector3 &p_center, const Vector3 &p_normal) {
	if (painting_mesh.is_null() && painting_scene.is_null()) {
		return 0;
	}

	if (rng.is_null()) {
		rng.instantiate();
	}
	if (random_seed != 0) {
		rng->set_seed((uint64_t)random_seed);
	} else {
		rng->randomize();
	}

	Vector3 normal = align_to_surface ? p_normal : Vector3(0, 1, 0);
	if (normal.length_squared() < CMP_EPSILON2) {
		normal = Vector3(0, 1, 0);
	}
	normal.normalize();

	Basis surface_basis;
	if (align_to_surface) {
		surface_basis = _basis_from_normal(normal);
	}

	const real_t radius = MAX(brush_radius, 0.001);
	const real_t min_dist = MAX(spacing, 0.001);

	const int max_count = CLAMP(int((Math::PI * radius * radius) / (min_dist * min_dist)), 1, 4096);

	int added = 0;
	for (int i = 0; i < max_count; i++) {
		const real_t ang = rng->randf() * Math::TAU;
		const real_t rad = radius * Math::sqrt(rng->randf());
		const real_t ca = Math::cos(ang);
		const real_t sa = Math::sin(ang);

		Vector3 offset;
		if (align_to_surface) {
			offset = surface_basis.get_column(0) * (rad * ca) + surface_basis.get_column(2) * (rad * sa);
		} else {
			offset = Vector3(rad * ca, 0, rad * sa);
		}

		const Vector3 point = p_center + offset;

		bool ok = true;
		for (int j = 0; j < instances.size(); j++) {
			if (point.distance_to(((Transform3D)(instances[j])).origin) < min_dist) {
				ok = false;
				break;
			}
		}
		if (!ok) {
			continue;
		}

		Transform3D t;
		t.origin = point;

		Basis b = align_to_surface ? surface_basis : Basis();
		const real_t yaw = Math::deg_to_rad(rng->randf_range(-random_rotation_degrees, random_rotation_degrees));
		b = b.rotated(align_to_surface ? normal : Vector3(0, 1, 0), yaw);
		const real_t tilt_x = Math::deg_to_rad(rng->randf_range(-random_tilt_degrees, random_tilt_degrees));
		b = b.rotated(b.get_column(0), tilt_x);
		const real_t tilt_z = Math::deg_to_rad(rng->randf_range(-random_tilt_degrees, random_tilt_degrees));
		b = b.rotated(b.get_column(2), tilt_z);

		const real_t scale = rng->randf_range(scale_min, scale_max);
		b = b.scaled(Vector3(scale, scale, scale));

		t.basis = b;
		instances.append(t);
		added++;
	}

	rebuild();
	return added;
}

int ScatterPainter3D::erase_near(const Vector3 &p_center, float p_radius) {
	int removed = 0;
	for (int i = instances.size() - 1; i >= 0; i--) {
		const Transform3D t = (Transform3D)(instances[i]);
		if (t.origin.distance_to(p_center) <= p_radius) {
			instances.remove_at(i);
			removed++;
		}
	}
	if (removed > 0) {
		rebuild();
	}
	return removed;
}

int ScatterPainter3D::clear_instances() {
	int previous_count = instances.size();
	instances.clear();
	notify_property_list_changed();
	rebuild();
	return previous_count;
}

void ScatterPainter3D::rebuild() {
	_rebuild_multimesh();

	if (scene_instances_dirty) {
		return;
	}
	scene_instances_dirty = true;
	call_deferred(SNAME("_rebuild_scene_instances_deferred"));
}

AABB ScatterPainter3D::get_aabb() const {
	if (instances.is_empty()) {
		if (painting_mesh.is_valid()) {
			AABB mesh_aabb = painting_mesh->get_aabb();
			return mesh_aabb;
		}
		return AABB(Vector3(), Vector3(1, 1, 1));
	}

	Vector3 extents(0.5, 0.5, 0.5);
	if (painting_mesh.is_valid()) {
		extents = painting_mesh->get_aabb().size * 0.5;
	}
	extents *= scale_max;

	AABB aabb;
	for (int i = 0; i < instances.size(); i++) {
		const Vector3 origin = ((Transform3D)(instances[i])).origin;
		if (i == 0) {
			aabb.position = origin - extents;
			aabb.size = extents * 2.0;
		} else {
			aabb.expand_to(origin - extents);
			aabb.expand_to(origin + extents);
		}
	}

	return aabb;
}

ScatterPainter3D::ScatterPainter3D() {
}