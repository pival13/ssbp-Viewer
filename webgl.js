import vertexCode from './vertexShader.js';
import fragmentCode from './fragmentShader.js';

let gl;
export const quad = {};

export function initialize(canvas) {
    canvas = document.getElementById("drawer");
    gl = canvas.getContext("webgl2");
    //gl = (new WebGL2RenderingContext());
    
    if (gl === null)
        throw "Unable to initialize WebGL. Your browser or machine may not support it.";

    const vertex = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertex, vertexCode);
    gl.compileShader(vertex);
    if (!gl.getShaderParameter(vertex, gl.COMPILE_STATUS))
        throw 'An error occurred while compiling vertex shader: ' + gl.getShaderInfoLog(vertex);;
    const fragment = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragment, fragmentCode);
    gl.compileShader(fragment);
    if (!gl.getShaderParameter(fragment, gl.COMPILE_STATUS))
        throw 'An error occurred while compiling fragment shader: ' + gl.getShaderInfoLog(fragment);;
    quad.id = gl.createProgram();
    gl.attachShader(quad.id, vertex);
    gl.attachShader(quad.id, fragment);
    gl.linkProgram(quad.id);
    if (!gl.getProgramParameter(quad.id, gl.LINK_STATUS))
        throw 'Unable to initialize the shader program: ' + gl.getProgramInfoLog(quad.id);
    gl.deleteShader(vertex);
    gl.deleteShader(fragment);

    quad.VAO = gl.createVertexArray();
    quad.VBO = gl.createBuffer();
    gl.bindVertexArray(quad.VAO);
    gl.bindBuffer(gl.ARRAY_BUFFER, quad.VBO);
    gl.bufferData(gl.ARRAY_BUFFER, 4 * 9 * 4, gl.DYNAMIC_DRAW);// sizeof(float) * elemPerVector * nbVertex
    gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(0);
    gl.bindAttribLocation(quad.id, 0, '_vertex');
    gl.vertexAttribPointer(1, 2, gl.FLOAT, false, 0, 4 * 3 * 4);
    gl.enableVertexAttribArray(1);
    gl.bindAttribLocation(quad.id, 1, '_uv');
    gl.vertexAttribPointer(2, 4, gl.FLOAT, false, 0, 4 * 5 * 4);
    gl.enableVertexAttribArray(2);
    gl.bindAttribLocation(quad.id, 2, '_color');
    gl.linkProgram(quad.id);

    gl.clearColor(0.5, 0.5, 0.5, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);

    quad.setInt     = function (name, v) { gl.useProgram(quad.id); gl.uniform1i(gl.getUniformLocation(quad.id, name), v); };
    quad.setFloat   = function (name, v) { gl.useProgram(quad.id); gl.uniform1f(gl.getUniformLocation(quad.id, name), v); };
    quad.setvec2    = function (name, v) { gl.useProgram(quad.id); gl.uniform2f(gl.getUniformLocation(quad.id, name), v.x, x.y); };
    quad.setvec3    = function (name, v) { gl.useProgram(quad.id); gl.uniform3f(gl.getUniformLocation(quad.id, name), v.x, v.y, v.z); };
    quad.setvec4    = function (name, v) { gl.useProgram(quad.id); gl.uniform4f(gl.getUniformLocation(quad.id, name), v.x, v.y, v.z, v.w); };
    quad.setmat4    = function (name, v) { gl.useProgram(quad.id); (new WebGL2RenderingContext()).uniformMatrix4fv(gl.getUniformLocation(quad.id, name), false, new Float32Array(v)); };
    quad.setTexture = function (name, v) {
        gl.useProgram(quad.id);
        gl.uniform1i(gl.getUniformLocation(quad.id, name), quad.textureId);
        gl.activeTexture(gl.TEXTURE0 + quad.textureId);
        quad.textureId += 1;
        gl.bindTexture(gl.TEXTURE_2D, v);//TODO
    };
    quad.draw = function (vertices, uvs, colors) {
        gl.useProgram(quad.id);
        gl.bindVertexArray(quad.VAO);
        gl.bindBuffer(gl.ARRAY_BUFFER, quad.VBO);
        gl.bufferSubData(gl.ARRAY_BUFFER, 0,  new Float32Array(vertices));
        gl.bufferSubData(gl.ARRAY_BUFFER, 48, new Float32Array(uvs));
        gl.bufferSubData(gl.ARRAY_BUFFER, 80, new Float32Array(colors));
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
        quad.textureId = 0
    };
}
