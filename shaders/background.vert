#version 330 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_UV;

out vec2 uv;
uniform vec2 u_Coef;

void main() {
    // Change the following value to change the background behaviour
    switch (1) {
    case 0:             // The background is stretch to match the window
        uv = a_UV;
        break;
    case 1:             // The background keep its size, and is repeated when necessary
        uv = a_UV / u_Coef;
        break;
    case 2:             // The background keep its ratio, but is scale to fully cover the window
        uv = vec2(
            a_UV.x * (u_Coef.x < u_Coef.y ? 1 : u_Coef.y / u_Coef.x),
            a_UV.y * (u_Coef.x < u_Coef.y ? u_Coef.x / u_Coef.y : 1)
        );
    }
    //uv += 

    gl_Position = vec4(a_Pos.x, -a_Pos.y, a_Pos.z, 1.0f);
}
