#include "SceneElement.h"
#include "scene/SceneContext.h"
#include "rendering/imgui/ImGuiManager.h"

#include <algorithm>
#include <cmath>

namespace {
    // Solution: I: Optional Catmull-Rom path smoothing.
    glm::vec3 catmull_rom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
        const float t2 = t * t;
        const float t3 = t2 * t;
        // Catmull-Rom spline interpolation
        // Formula adapted from Robert Dunlop, "Introduction to Catmull-Rom Splines":
        // https://mvps.org/directx/articles/catmull/
        return 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * t +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
        );
    }
}

void EditorScene::SceneElement::add_imgui_edit_section(MasterRenderScene& /*render_scene*/, const SceneContext& /*scene_context*/) {
    ImGui::InputText("Name", &name, 0);
    ImGui::Spacing();
}

void EditorScene::SceneElement::visit_children_recursive(const std::function<void(SceneElement&)>& fn) const {
    auto children = get_children();
    if (children == nullptr) return;
    for (auto& child: *children) {
        fn(*child);
        child->visit_children_recursive(fn);
    }
}

json EditorScene::SceneElement::texture_to_json(const std::shared_ptr<TextureHandle>& texture) {
    if (!texture->get_filename().has_value()) {
        return {
            {"error", Formatter() << "Texture does not have a filename so can not be exported, and has been skipped."}
        };
    }

    return {
        {"filename",   texture->get_filename().value()},
        {"is_srgb",    texture->is_srgb()},
        {"is_flipped", texture->is_flipped()},
    };
}

std::shared_ptr<TextureHandle> EditorScene::SceneElement::texture_from_json(const SceneContext& scene_context, const json& json) {
    if (json.contains("error")) {
        return scene_context.texture_loader.default_white_texture();
    }

    return scene_context.texture_loader.load_from_file(json["filename"], json["is_srgb"], json["is_flipped"]);
}

void EditorScene::LocalTransformComponent::add_local_transform_imgui_edit_section(MasterRenderScene& /*render_scene*/, const SceneContext& scene_context) {
    ImGui::Text("Local Transformation");
    bool transformUpdated = false;
    transformUpdated |= ImGui::DragFloat3("Translation", &position[0], 0.01f);
    ImGui::DragDisableCursor(scene_context.window);

    glm::vec3 euler_rotation_degrees = glm::degrees(euler_rotation);
    transformUpdated |= ImGui::DragFloat3("Rotation", &euler_rotation_degrees[0]);
    ImGui::DragDisableCursor(scene_context.window);
    euler_rotation = glm::radians(glm::mod(euler_rotation_degrees, 360.0f)); // euler_rotation for task B

    {
        // Static also means that all [EntityElement] will share the value
        static bool lock_scale = true;

        glm::vec3 temp_scale = scale;
        if (ImGui::DragFloat3("Scale", &temp_scale[0], 0.01f)) {
            transformUpdated = true;

            if (lock_scale) {
                // Assume only channel can change at a time (I think this is true based on how ImGui works?)
                if (temp_scale.x != scale.x) {
                    if (scale.x == 0.0f) {
                        scale = glm::vec3(temp_scale.x);
                    } else {
                        scale *= temp_scale.x / scale.x;
                    }
                } else if (temp_scale.y != scale.y) {
                    if (scale.y == 0.0f) {
                        scale = glm::vec3(temp_scale.y);
                    } else {
                        scale *= temp_scale.y / scale.y;
                    }
                } else if (temp_scale.z != scale.z) {
                    if (scale.z == 0.0f) {
                        scale = glm::vec3(temp_scale.z);
                    } else {
                        scale *= temp_scale.z / scale.z;
                    }
                }
            } else {
                scale = temp_scale;
            }
        }

        ImGui::SameLine();
        ImGui::Checkbox("[Lock]", &lock_scale);
    }
    ImGui::Spacing();

    if (transformUpdated) {
        update_instance_data();
    }

    // Solution: I: Path animation controls.
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("New feature: Path Animation");

    if (ImGui::Button("Add Current Position")) {
        path_points.push_back(position);
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear Path")) {
        path_points.clear();
        path_animation_playing = false;
        path_animation_time = 0.0f;
    }

    ImGui::Checkbox("Loop Path", &path_animation_loop);
    ImGui::Checkbox("Curved Path", &path_curve_enabled);
    ImGui::Checkbox("Mouse Ground Add", &path_mouse_ground_add_enabled);
    ImGui::SameLine();
    ImGui::HelpMarker("When enabled, Shift + left click on the ground plane to add a path point");
    ImGui::DragFloat("Path Duration", &path_animation_duration, 0.1f, 0.1f, FLT_MAX, "%.2f sec");

    if (path_points.size() >= 2) {
        if (ImGui::SliderFloat("Path Time", &path_animation_time, 0.0f, path_animation_duration, "%.2f sec")) {
            update_path_animation(0.0f, true);
        }

        if (ImGui::Button(path_animation_playing ? "Pause Path" : "Play Path")) {
            path_animation_playing = !path_animation_playing;
        }

        ImGui::SameLine();

        if (ImGui::Button("Restart Path")) {
            path_animation_time = 0.0f;
            path_animation_playing = true;
            position = path_points.front();
            update_instance_data();
        }
    } else {
        ImGui::Text("Add at least two points to play a path.");
    }

    for (auto i = 0u; i < path_points.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        bool point_changed = ImGui::DragFloat3("Point", &path_points[i][0], 0.01f);
        ImGui::DragDisableCursor(scene_context.window);

        ImGui::SameLine();

        if (ImGui::Button("Remove")) {
            path_points.erase(path_points.begin() + i);
            if (path_points.size() < 2) {
                path_animation_playing = false;
                path_animation_time = 0.0f;
            }
            ImGui::PopID();
            break;
        }

        if (point_changed && path_animation_time == 0.0f && i == 0) {
            position = path_points.front();
            update_instance_data();
        }

        ImGui::PopID();
    }
}

void EditorScene::LocalTransformComponent::update_path_animation(float delta_time, bool force_update) {
    // Solution: I: Advance or preview the stored path.
    if ((!path_animation_playing && !force_update) || path_points.size() < 2) {
        return;
    }

    path_animation_duration = std::max(path_animation_duration, 0.1f);
    if (path_animation_playing) {
        path_animation_time += delta_time;
    }

    if (path_animation_time >= path_animation_duration) {
        if (path_animation_loop) {
            path_animation_time = std::fmod(path_animation_time, path_animation_duration);
        } else {
            path_animation_time = path_animation_duration;
            path_animation_playing = false;
        }
    }

    const float path_progress = path_animation_time / path_animation_duration;
    const float scaled_progress = path_progress * static_cast<float>(path_points.size() - 1);
    // Solution: I: Convert path time into segment progress.
    const auto start_index = static_cast<std::size_t>(std::min(
        std::floor(scaled_progress),
        static_cast<float>(path_points.size() - 2)
    ));
    const float segment_progress = scaled_progress - static_cast<float>(start_index);

    if (path_curve_enabled && path_points.size() >= 3) {
        const glm::vec3& p0 = path_points[start_index == 0 ? start_index : start_index - 1];
        const glm::vec3& p1 = path_points[start_index];
        const glm::vec3& p2 = path_points[start_index + 1];
        const glm::vec3& p3 = path_points[std::min(start_index + 2, path_points.size() - 1)];
        position = catmull_rom(p0, p1, p2, p3, segment_progress);
    } else {
        position = glm::mix(path_points[start_index], path_points[start_index + 1], segment_progress);
    }
    update_instance_data();
}

// Solution: task B
glm::mat4 EditorScene::LocalTransformComponent::calc_model_matrix() const {
    return glm::translate(position) * 
    glm::rotate(euler_rotation.z, glm::vec3{0.0f, 0.0f, 1.0f}) *
    glm::rotate(euler_rotation.y, glm::vec3{0.0f, 1.0f, 0.0f}) *
    glm::rotate(euler_rotation.x, glm::vec3{1.0f, 0.0f, 0.0f}) *
    glm::scale(scale);
}

void EditorScene::LocalTransformComponent::update_local_transform_from_json(const json& json) {
    auto t = json["local_transform"];
    position = t["position"];
    euler_rotation = t["euler_rotation"];
    scale = t["scale"];

    // Solution: I: Load optional path data.
    path_points.clear();
    path_animation_playing = false;
    path_animation_time = 0.0f;
    path_animation_loop = true;
    path_curve_enabled = false;
    path_mouse_ground_add_enabled = false;

    if (json.contains("path_animation")) {
        const auto& path = json["path_animation"];
        if (path.contains("points")) {
            path_points = path["points"].get<std::vector<glm::vec3>>();
        }
        if (path.contains("loop")) {
            path_animation_loop = path["loop"];
        }
        if (path.contains("curved")) {
            path_curve_enabled = path["curved"];
        }
        if (path.contains("mouse_ground_add")) {
            path_mouse_ground_add_enabled = path["mouse_ground_add"];
        }
        if (path.contains("duration")) {
            path_animation_duration = std::max(path["duration"].get<float>(), 0.1f);
        }
        if (path.contains("time")) {
            path_animation_time = std::clamp(path["time"].get<float>(), 0.0f, path_animation_duration);
        }
    }
}

json EditorScene::LocalTransformComponent::local_transform_into_json() const {
    return {"local_transform", {
        {"position", position},
        {"euler_rotation", euler_rotation},
        {"scale", scale},
    }};
}

json EditorScene::LocalTransformComponent::path_animation_into_json() const {
    // Solution: I: Save path points and playback options.
    return {"path_animation", {
        {"points", path_points},
        {"loop", path_animation_loop},
        {"curved", path_curve_enabled},
        {"mouse_ground_add", path_mouse_ground_add_enabled},
        {"duration", path_animation_duration},
        {"time", path_animation_time},
    }};
}

void EditorScene::LitMaterialComponent::add_material_imgui_edit_section(MasterRenderScene& /*render_scene*/, const SceneContext& /*scene_context*/) {
    // Set this to true if the user has changed any of the material values, otherwise the changes won't be propagated
    bool material_changed = false;
    ImGui::Text("Material");

    // Add UI controls here //Solution: D (1/2)
    material_changed |= ImGui::ColorEdit3("Diffuse Tint", &material.diffuse_tint[0]);
    material_changed |= ImGui::DragFloat("Diffuse Factor", &material.diffuse_tint.a, 0.01f, 0.0f, FLT_MAX);

    material_changed |= ImGui::ColorEdit3("Specular Tint", &material.specular_tint[0]);
    material_changed |= ImGui::DragFloat("Specular Factor", &material.specular_tint.a, 0.01f, 0.0f, FLT_MAX);

    material_changed |= ImGui::ColorEdit3("Ambient Tint", &material.ambient_tint[0]);
    material_changed |= ImGui::DragFloat("Ambient Factor", &material.ambient_tint.a, 0.01f, 0.0f, FLT_MAX);

    material_changed |= ImGui::DragFloat("Shininess", &material.shininess, 0.1f, 0.0f, FLT_MAX);

    // Solution: E (8/10)
    material_changed |= ImGui::DragFloat("Texture Scale", &material.texture_scale, 0.01f, 0.0f, FLT_MAX);



    ImGui::Spacing();
    if (material_changed) {
        update_instance_data();
    }
}

void EditorScene::LitMaterialComponent::update_material_from_json(const json& json) {
    auto m = json["material"];
    material.diffuse_tint = m["diffuse_tint"];
    material.specular_tint = m["specular_tint"];
    material.ambient_tint = m["ambient_tint"];
    material.shininess = m["shininess"];

    // Solution: E (9/10)
    if (m.contains("texture_scale")) {
    material.texture_scale = m["texture_scale"];
    }

}

json EditorScene::LitMaterialComponent::material_into_json() const {
    return {"material", {
        {"diffuse_tint", material.diffuse_tint},
        {"specular_tint", material.specular_tint},
        {"ambient_tint", material.ambient_tint},
        {"shininess", material.shininess},

        // Solution: E (10/10)
        {"texture_scale", material.texture_scale},

    }};
}

void EditorScene::EmissiveMaterialComponent::add_emissive_material_imgui_edit_section(MasterRenderScene& /*render_scene*/, const SceneContext& /*scene_context*/) {
    // Set this to true if the user has changed any of the material values, otherwise the changes won't be propagated
    bool material_changed = false;
    ImGui::Text("Emissive Material");

    // Add UI controls here // Solution: D (2/2)
    material_changed |= ImGui::ColorEdit3("Emission Tint", &material.emission_tint[0]);
    material_changed |= ImGui::DragFloat("Emission Factor", &material.emission_tint.a, 0.01f, 0.0f, FLT_MAX);


    ImGui::Spacing();
    if (material_changed) {
        update_instance_data();
    }
}

void EditorScene::EmissiveMaterialComponent::update_emissive_material_from_json(const json& json) {
    auto m = json["material"];
    material.emission_tint = m["emission_tint"];
}

json EditorScene::EmissiveMaterialComponent::emissive_material_into_json() const {
    return {"material", {
        {"emission_tint", material.emission_tint},
    }};
}

void EditorScene::AnimationComponent::add_animation_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& /*scene_context*/) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto entity = get_entity();
    const auto& animations = entity->get_animations();

    ImGui::Text("Animation");
    std::string selected_animation = "[NONE]";
    double ticks_per_second = 1.0;
    double duration_ticks = 0.0;
    if (get_animation_parameters().animation_id != NONE_ANIMATION) {
        std::tie(selected_animation, ticks_per_second, duration_ticks) = animations[get_animation_parameters().animation_id];
    }
    if (ImGui::BeginCombo("Animation Selection", selected_animation.c_str(), 0)) {
        for (auto i = 0u; i < animations.size(); ++i) {
            const auto& animation = animations[i];
            const bool is_selected = i == get_animation_parameters().animation_id;
            if (ImGui::Selectable(std::get<0>(animation).c_str(), is_selected)) {
                render_scene.animator.stop(entity);
                get_animation_parameters().animation_id = i;
                entity->get_animation_time_seconds() = 0.0;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable("[NONE]", get_animation_parameters().animation_id == NONE_ANIMATION)) {
            render_scene.animator.stop(entity);
            get_animation_parameters().animation_id = NONE_ANIMATION;
            entity->get_animation_time_seconds() = 0.0;
        }
        ImGui::EndCombo();

        entity->get_animation_id() = get_animation_parameters().animation_id;
    }
    if (get_animation_parameters().animation_id != NONE_ANIMATION) {
        std::tie(selected_animation, ticks_per_second, duration_ticks) = animations[get_animation_parameters().animation_id];

        auto float_time = (float) entity->get_animation_time_seconds();
        auto float_duration = (float) (duration_ticks / ticks_per_second);
        if (ImGui::SliderFloat("Animation Time (sec)", &float_time, 0.0f, float_duration, "%.3f", ImGuiSliderFlags_NoRoundToFormat)) {
            entity->get_animation_time_seconds() = float_time;
        }

        bool is_playing = render_scene.animator.is_animating(entity).has_value();

        if (ImGui::Button("Start")) {
            render_scene.animator.start(entity, get_animation_parameters());
        }

        ImGui::SameLine();

        if (!is_playing) ImGui::BeginDisabled();
        if (ImGui::Button("Pause")) {
            render_scene.animator.pause(entity);
        }
        if (!is_playing) ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Resume")) {
            render_scene.animator.resume(entity, get_animation_parameters());
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop")) {
            render_scene.animator.stop(entity);
        }

        ImGui::SameLine();

        if (ImGui::Checkbox("Loop", &get_animation_parameters().loop) && is_playing) {
            render_scene.animator.update_param(entity, get_animation_parameters());
        }

        auto float_speed = (float) get_animation_parameters().speed;
        if (ImGui::SliderFloat("Speed", &float_speed, 0.0, 10.0)) {
            get_animation_parameters().speed = float_speed;
            if (is_playing) {
                render_scene.animator.update_param(entity, get_animation_parameters());
            }
        }
    }
}

bool EditorScene::is_null(const ElementRef& ref) {
    return *((const void**) &ref) == nullptr;
}

bool EditorScene::eq(const ElementRef& e1, const ElementRef& e2) {
    return *((const void**) &e1) == *((const void**) &e2);
}
