#version 410 core
// Per vertex data
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;

// Solution: G (1/11): Shading
out VertexOut {
    vec3 ws_position;
    vec3 ws_normal;
    vec2 texture_coordinate;
} vertex_out;

// Per instance data
uniform mat4 model_matrix;
uniform mat3 normal_matrix;

// Global data
uniform mat4 projection_view_matrix;

void main() {
    // Transform vertices
    vec3 ws_position = (model_matrix * vec4(vertex_position, 1.0f)).xyz;
    vec3 ws_normal = normalize(normal_matrix * normal);
    // Solution: G (2/11): Shading
    vertex_out.ws_position = ws_position;
    vertex_out.ws_normal = ws_normal;
    vertex_out.texture_coordinate = texture_coordinate;

    gl_Position = projection_view_matrix * vec4(ws_position, 1.0f);
}
