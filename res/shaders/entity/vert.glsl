#version 410 core
#include "../common/lights.glsl"

// Per vertex data
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;
//solution G:delete the bone weight and bone index since it's not needed in the static entity shader
out VertexOut {
    vec3 ws_position;
    vec3 ws_normal;
    vec2 texture_coordinate;
} vertex_out;

// Per instance data
uniform mat4 model_matrix;
uniform mat3 normal_matrix;

//solution G:delete the material properties since it's not needed in the vertex shader
// // Material properties
// uniform vec3 diffuse_tint;
// uniform vec3 specular_tint;
// uniform vec3 ambient_tint;
// uniform float shininess;

// // Global data
// uniform vec3 ws_view_position;
uniform mat4 projection_view_matrix;

void main() {
    // Transform vertices
    vec3 ws_position = (model_matrix * vec4(vertex_position, 1.0f)).xyz;
    vec3 ws_normal = normalize(normal_matrix * normal);
    vertex_out.texture_coordinate = texture_coordinate;

    gl_Position = projection_view_matrix * vec4(ws_position, 1.0f);

    //solution G:delete the light calculation in the vertex shader, and pass the world space position and normal to the fragment shader, and do the lighting calculation in the fragment shader with the texture sampling.
    // // Per vertex lighting
    // vec3 ws_view_dir = normalize(ws_view_position - ws_position);
    // LightCalculatioData light_calculation_data = LightCalculatioData(ws_position, ws_view_dir, ws_normal);
    // Material material = Material(diffuse_tint, specular_tint, ambient_tint, shininess);
    
    //solution G:delete the light calculation in the vertex shader, and pass the world space position and normal to the fragment shader, and do the lighting calculation in the fragment shader with the texture sampling.
    vertex_out.ws_position = ws_position;
    vertex_out.ws_normal = ws_normal;
    vertex_out.texture_coordinate = texture_coordinate;

    //soluton G:delete the per vertex lighting calculation, and pass the world space position and normal to the fragment shader, and do the lighting calculation in the fragment shader with the texture sampling.
    // vertex_out.lighting_result = total_light_calculation(light_calculation_data, material
    //     #if NUM_PL > 0
    //     ,point_lights
    //     #endif
    //     #if NUM_DL > 0
    //     ,DirectionalLightData directional_lights[NUM_DL]
    //     #endif
    // );

}
