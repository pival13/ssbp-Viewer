R"(
#version 330 core

layout (location = 0) in vec3 _vertex;
layout (location = 1) in vec2 _uv;
layout (location = 2) in vec4 _color;

out vec4 vertexColor;
out vec2 uv;

uniform mat4 u_View;

void main()
{
    gl_Position = u_View * vec4(_vertex, 1);
    vertexColor = _color;
    uv = _uv;
}
)"