#version 330 core

in vec2 uv;
out vec4 fragColor;

uniform sampler2D u_Texture;
uniform bool u_UseTexture;

void main() {
    if (u_UseTexture) {
        fragColor = texture2D(u_Texture, uv);
    }
    else {
        //fragColor = vec4(0.7f, 0.4f, 0.2f, 1.0f);
        fragColor = vec4(0.35f, 0.4f, 0.5f, 1.0f);
    }
}
