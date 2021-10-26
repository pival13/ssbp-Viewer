export default `
precision mediump float;

varying vec4 vertexColor;
varying vec2 uv;

uniform sampler2D u_Texture;
uniform int u_BlendType;

void main() {
    vec4 color = vec4(1,1,1,1);// texture2D(u_Texture, uv);

    gl_FragColor = color * vertexColor;
    if (u_BlendType == 2) // Add
        gl_FragColor.a *= 0.5; // Decreasing opacity do not change the color, but improve the render when exported with alpha
}`;