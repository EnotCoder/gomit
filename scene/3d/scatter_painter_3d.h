/**************************************************************************/
/*  scatter_painter_3d.h                                                  */
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

#include "core/math/random_number_generator.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/mesh.h"
#include "scene/resources/packed_scene.h"

class MultiMeshInstance3D;

class ScatterPainter3D : public Node3D {
	GDCLASS(ScatterPainter3D, Node3D);

	Ref<Mesh> painting_mesh;
	Ref<PackedScene> painting_scene;

	Array instances;

	// Brush / distribution settings.
	float brush_radius = 5.0;
	float spacing = 1.0;
	float scale_min = 0.8;
	float scale_max = 1.2;
	float random_rotation_degrees = 45.0;
	float random_tilt_degrees = 10.0;
	bool align_to_surface = true;
	int random_seed = 0;

	Ref<RandomNumberGenerator> rng;

	MultiMeshInstance3D *multimesh_instance = nullptr;
	Node3D *scene_instances_root = nullptr;
	bool scene_instances_dirty = false;

	void _ensure_internal_nodes();
	void _rebuild_multimesh();
	void _rebuild_scene_instances();
	void _rebuild_scene_instances_deferred();
	Basis _basis_from_normal(const Vector3 &p_normal) const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_painting_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_painting_mesh() const;

	void set_painting_scene(const Ref<PackedScene> &p_scene);
	Ref<PackedScene> get_painting_scene() const;

	void set_instances(const Array &p_instances);
	Array get_instances() const;

	void set_brush_radius(float p_radius);
	float get_brush_radius() const;

	void set_spacing(float p_spacing);
	float get_spacing() const;

	void set_scale_min(float p_scale);
	float get_scale_min() const;

	void set_scale_max(float p_scale);
	float get_scale_max() const;

	void set_random_rotation_degrees(float p_degrees);
	float get_random_rotation_degrees() const;

	void set_random_tilt_degrees(float p_degrees);
	float get_random_tilt_degrees() const;

	void set_align_to_surface(bool p_align);
	bool is_align_to_surface() const;

	void set_random_seed(int p_seed);
	int get_random_seed() const;

	int get_instance_count() const;

	int paint_stroke(const Vector3 &p_center, const Vector3 &p_normal);
	int erase_near(const Vector3 &p_center, float p_radius);
	int clear_instances();
	void rebuild();

	AABB get_aabb() const;

	ScatterPainter3D();
};