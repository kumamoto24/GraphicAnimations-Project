#version 410 core
#include "../common/lights.glsl"
uniform vec3 ws_view_position;

uniform vec3 diffuse_tint;
uniform vec3 specular_tint;
uniform vec3 ambient_tint;
uniform float shininess;

//solution G change the lighting_result into the world space position and normal, and pass the texture
in VertexOut {
    vec3 ws_position;
    vec3 ws_normal;
    vec2 texture_coordinate;
} frag_in;

layout(location = 0) out vec4 out_colour;

// Global Data
uniform float inverse_gamma;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_map_texture;

//sloution G:add the point light data from vert shader
#if NUM_PL > 0
layout (std140) uniform PointLightArray {
    PointLightData point_lights[NUM_PL];
};
#endif
//sloution H:add the direcional light data
#if NUM_DL > 0
layout (std140) uniform DirectionalLightArray {
    DirectionalLightData directional_lights[NUM_DL];
};
#endif

void main() {
    //solution G:copy the caculationg of light from vert.glsl, and change the element form the vertex_out to frag_in, and do the lighting calculation in the fragment shader with the texture sampling.
   // Calculate lighting per fragment, then apply texture sampling.
   //copy from the vert.glsl and change the element form the vertex_out to frag_in
    vec3 ws_view_dir = normalize(ws_view_position - frag_in.ws_position);

    LightCalculatioData light_calculation_data =
        LightCalculatioData(
            frag_in.ws_position,
            ws_view_dir,
            normalize(frag_in.ws_normal)
        );

    Material material =
        Material(diffuse_tint, specular_tint, ambient_tint, shininess);

    //solution H:add the direcional light data
    LightingResult lighting_result =
        total_light_calculation(
            light_calculation_data,
            material
            #if NUM_PL > 0
            ,point_lights
            #endif
            //solution H:add the direcional light data
            #if NUM_DL > 0
            ,directional_lights
            #endif
        );

    vec3 resolved_lighting =
        resolve_textured_light_calculation(
            lighting_result,
            diffuse_texture,
            specular_map_texture,
            frag_in.texture_coordinate
        );

        out_colour = vec4(resolved_lighting, 1.0f);
        out_colour.rgb = pow(out_colour.rgb, vec3(inverse_gamma));
}


