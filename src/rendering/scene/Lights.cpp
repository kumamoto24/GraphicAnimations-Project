#include "Lights.h"

#include <algorithm>

std::vector<PointLight> LightScene::get_nearest_point_lights(glm::vec3 target, size_t max_count, size_t min_count) const {
    return get_nearest_lights(point_lights, target, max_count, min_count);
}

std::vector<DirectionalLight> LightScene::get_directional_lights() const {
    std::vector<DirectionalLight> result{};
    result.reserve(directional_lights.size());

    for (const auto& directional_light : directional_lights) {
        result.push_back(*directional_light);
    }

    return result;
}

template<typename Light>
std::vector<Light> LightScene::get_nearest_lights(const std::unordered_set<std::shared_ptr<Light>>& lights, glm::vec3 target, size_t max_count, size_t min_count) {
    if (lights.size() <= max_count) {
        std::vector<Light> result{};
        result.reserve(std::max(lights.size(), min_count));

        for (const auto& point_light : lights) {
            result.push_back(*point_light);
        }

        while (result.size() < min_count) {
            result.push_back(Light::off());
        }

        return result;
    }

    size_t result_count = std::min(lights.size(), max_count);

    std::vector<std::pair<float, Light>> sorted_vector{};
    sorted_vector.reserve(lights.size());

    for (const auto& point_light : lights) {
        glm::vec3 diff = point_light->position - target;
        float distance_squared = glm::dot(diff, diff);
        sorted_vector.emplace_back(distance_squared, *point_light);
    }

    std::partial_sort(
        sorted_vector.begin(),
        sorted_vector.begin() + (long) result_count,
        sorted_vector.end(),
        [](const std::pair<float, Light>& lhs, const std::pair<float, Light>& rhs) -> bool {
            return lhs.first < rhs.first;
        }
    );

    std::vector<Light> result{};
    result.reserve(std::max(result_count, min_count));

    for (auto i = 0u; i < result_count; ++i) {
        result.push_back(sorted_vector[i].second);
    }

    while (result.size() < min_count) {
        result.push_back(Light::off());
    }

    return result;
}