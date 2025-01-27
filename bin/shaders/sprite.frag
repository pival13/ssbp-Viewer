#version 330 core

in vec4 vertexColor;
in vec2 uv;

out vec4 fragColor;

//uniform float u_Opacity;
uniform bool u_UseTexture;
uniform sampler2D u_Texture;
uniform int u_BlendType;

void main() {
    if (!u_UseTexture) {
        discard;
    }

    vec4 color = texture2D(u_Texture, uv);

    fragColor = color * vertexColor;
    if (u_BlendType == 2) // Add
        fragColor.a *= 0.5f; // Decreasing opacity do not change the color, but improve the render when exported with alpha
}
