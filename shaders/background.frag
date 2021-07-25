#version 330 core

in vec2 uv;
out vec4 fragColor;

uniform sampler2D u_Texture;
uniform bool u_UseTexture;

void main() {
    if (u_UseTexture)
        fragColor = texture2D(u_Texture, uv);
    else
        fragColor = vec4(1.0f);
}
