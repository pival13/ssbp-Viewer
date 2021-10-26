export default `
precision mediump float;

varying vec4 vertexColor;
varying vec2 uv;

uniform bool u_UseTexture;
uniform sampler2D u_Texture;
uniform int u_BlendType;

void main() {
    vec4 color = u_UseTexture ? texture2D(u_Texture, uv) : vec4(1,1,1,1);

    gl_FragColor = color * vertexColor;
    if (u_BlendType == 2) // Add
        gl_FragColor.a *= 0.5; // Decreasing opacity do not change the color, but improve the render when exported with alpha
}`;