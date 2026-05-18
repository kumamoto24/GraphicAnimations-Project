#include "DirectionalLightElement.h"

#include <limits>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

namespace {
    glm::vec3 safe_direction(glm::vec3 direction) {
        if (glm::dot(direction, direction) <= 0.000001f) {
            return {0.0f, -1.0f, 0.0f};
        }
        return glm::normalize(direction);
    }

    glm::mat4 direction_visual_transform(glm::vec3 position, glm::vec3 direction, float visual_scale) {
        // Solution: H (12/16): Rotate a cone from its local +Y axis so the in-world marker points along light travel.
        glm::quat rotation = glm::rotation(glm::vec3{0.0f, 1.0f, 0.0f}, safe_direction(direction));
        return glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(glm::vec3{0.25f, 0.75f, 0.25f} * visual_scale);
    }

    glm::vec3 normalised_light_colour(glm::vec4 colour) {
        float max_component = glm::compMax(glm::vec3(colour));
        if (max_component <= 0.000001f) {
            return glm::vec3{0.0f};
        }
        return glm::vec3(colour) / max_component;
    }
}

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::new_default(const SceneContext& scene_context, EditorScene::ElementRef parent) {
    auto light_element = std::make_unique<DirectionalLightElement>(
        parent,
        "New Directional Light",
        glm::vec3{0.0f, 2.0f, 0.0f},
        glm::vec3{-1.0f, -1.0f, -1.0f},
        DirectionalLight::create(
            glm::vec3{-1.0f, -1.0f, -1.0f},
            glm::vec4{1.0f}
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("cone.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{}, // Set via update_instance_data()
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
        )
    );

    light_element->update_instance_data();
    return light_element;
}

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::from_json(const SceneContext& scene_context, EditorScene::ElementRef parent, const json& j) {
    auto light_element = new_default(scene_context, parent);

    light_element->position = j["position"];
    light_element->direction = j["direction"];
    light_element->light->colour = j["colour"];
    light_element->visible = j["visible"];
    light_element->visual_scale = j["visual_scale"];

    light_element->update_instance_data();
    return light_element;
}

json EditorScene::DirectionalLightElement::into_json() const {
    // Solution: H (16/16): Persist direction-specific light data so scene save/load restores the same illumination.
    return {
        {"position",     position},
        {"direction",    direction},
        {"colour",       light->colour},
        {"visible",      visible},
        {"visual_scale", visual_scale},
    };
}

void EditorScene::DirectionalLightElement::add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) {
    ImGui::Text("Directional Light");
    SceneElement::add_imgui_edit_section(render_scene, scene_context);

    ImGui::Text("Local Transformation");
    bool transform_updated = false;
    transform_updated |= ImGui::DragFloat3("Translation", &position[0], 0.01f);
    transform_updated |= ImGui::DragFloat3("Direction", &direction[0], 0.01f);
    ImGui::DragDisableCursor(scene_context.window);
    ImGui::Spacing();

    ImGui::Text("Light Properties");
    transform_updated |= ImGui::ColorEdit3("Colour", &light->colour[0]);
    ImGui::Spacing();
    transform_updated |= ImGui::DragFloat("Intensity", &light->colour.a, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragDisableCursor(scene_context.window);

    ImGui::Spacing();
    ImGui::Text("Visuals");

    transform_updated |= ImGui::Checkbox("Show Visuals", &visible);
    transform_updated |= ImGui::DragFloat("Visual Scale", &visual_scale, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragDisableCursor(scene_context.window);

    if (transform_updated) {
        update_instance_data();
    }
}

void EditorScene::DirectionalLightElement::update_instance_data() {
    transform = glm::translate(position);

    glm::vec3 world_direction = safe_direction(direction);
    if (!EditorScene::is_null(parent)) {
        // Post multiply by transform so that local transformations are applied first.
        transform = (*parent)->transform * transform;
        world_direction = safe_direction(glm::mat3((*parent)->transform) * world_direction);
    }

    light->direction = world_direction;

    if (visible) {
        direction_indicator->instance_data.model_matrix = direction_visual_transform(glm::vec3(transform[3]), world_direction, visual_scale);
    } else {
        // Throw off to infinity as a hacky way to make model invisible.
        direction_indicator->instance_data.model_matrix = glm::scale(glm::vec3{std::numeric_limits<float>::infinity()}) * glm::translate(glm::vec3{std::numeric_limits<float>::infinity()});
    }

    direction_indicator->instance_data.material.emission_tint = glm::vec4(normalised_light_colour(light->colour), direction_indicator->instance_data.material.emission_tint.a);
}

const char* EditorScene::DirectionalLightElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}
