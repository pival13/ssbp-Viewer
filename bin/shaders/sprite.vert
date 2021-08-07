#version 330 core
out vec4 vertexColor;
out vec2 uv;

struct Quad {
    vec4 vertex;
    vec2 uv;
    vec4 color;
};

uniform mat4 u_View;
uniform Quad u_Quad[4];

void main()
{
    gl_Position = u_View * u_Quad[gl_VertexID].vertex;
    vertexColor = u_Quad[gl_VertexID].color;
    uv =          u_Quad[gl_VertexID].uv;
}
