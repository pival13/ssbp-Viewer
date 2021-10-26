export default `
attribute vec3 _vertex;
attribute vec2 _uv;
attribute vec4 _color;

varying vec4 vertexColor;
varying vec2 uv;

uniform mat4 u_View;

void main()
{
    gl_Position = u_View * vec4(_vertex, 1);
    vertexColor = _color;
    uv = _uv;
}`;