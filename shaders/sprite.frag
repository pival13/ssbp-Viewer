#version 330 core

in vec4 vertexColor;
in vec2 uv;

out vec4 fragColor;

//uniform float u_Opacity;
uniform bool u_UseTexture;
uniform sampler2D u_Texture;
uniform int u_BlendType;

void main() {
    vec4 color = texture2D(u_Texture, uv);

    if (!u_UseTexture) {
        discard;
    }

    fragColor = color * vertexColor;
    //fragColor.a *= u_Opacity;
    if (u_BlendType == 2) // Add
        fragColor.a *= 0.5f; // Decreasing opacity do not change the color, but improve the render without background
    //fragColor = vec4(1);
}