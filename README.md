# GraphicAnimations-Project

# Task G: Per-Fragment Lighting
# Solution

## 1. Move Light Defines to the Fragment Shader

the point light count define was sent to the vertex shader:

```cpp
set_vert_define("NUM_PL", Formatter() << count);
```

This was changed to:

```cpp
set_frag_define("NUM_PL", Formatter() << count);
```
# 2. Move PointLight Uniforms from Vertex Shader to Fragment Shader

The following uniform block was originally located in `vert.glsl`:

```glsl
#if NUM_PL > 0
layout (std140) uniform PointLightArray {
    PointLightData point_lights[NUM_PL];
};
#endif
```

It was moved into `frag.glsl`.

---

# 3. Change Vertex Shader Outputs

Originally, the vertex shader output contained a precomputed lighting result:

```glsl
out VertexOut {
    LightingResult lighting_result;
    vec2 texture_coordinate;
} vertex_out;
```

This was changed to:

```glsl
out VertexOut {
    vec3 ws_position;
    vec3 ws_normal;
    vec2 texture_coordinate;
} vertex_out;
```

The old lighting calculation:

```glsl
vertex_out.lighting_result = total_light_calculation(...);
```

was removed and replaced with:

```glsl
vertex_out.ws_position = ws_position;
vertex_out.ws_normal = ws_normal;
vertex_out.texture_coordinate = texture_coordinate;
```

---

# 4. Change Fragment Shader Inputs

the fragment shader received a completed lighting result:

```glsl
in VertexOut {
    LightingResult lighting_result;
    vec2 texture_coordinate;
} frag_in;
```

This was changed to:

```glsl
in VertexOut {
    vec3 ws_position;
    vec3 ws_normal;
    vec2 texture_coordinate;
} frag_in;
```

---

# 5. Perform Lighting Calculations in the Fragment Shader

the fragment shader directly used:

```glsl
frag_in.lighting_result
```

lighting is calculated inside the fragment shader now:

```glsl
vec3 ws_view_dir = normalize(ws_view_position - frag_in.ws_position);

LightCalculatioData light_calculation_data =
    LightCalculatioData(
        frag_in.ws_position,
        ws_view_dir,
        normalize(frag_in.ws_normal)
    );

Material material =
    Material(diffuse_tint, specular_tint, ambient_tint, shininess);

LightingResult lighting_result =
    total_light_calculation(
        light_calculation_data,
        material
        #if NUM_PL > 0
        ,point_lights
        #endif
    );
```

The old resolve call:

```glsl
resolve_textured_light_calculation(
    frag_in.lighting_result,
    ...
)
```

was changed to:

```glsl
resolve_textured_light_calculation(
    lighting_result,
    ...
)
```

# Result

After these modifications:

- Lighting is calculated in fragment instead of vertex

