import {loadSsbp} from './loadSsbp.js';
import {print} from './printer.js';
import {initialize as initWebGL, quad, loadTexture, clear} from './webgl.js';
import {translate, rotate, scale, identity} from './math.js';

console.log(scale)

const canvas = document.getElementById("drawer");

initWebGL(canvas);

quad.setMat4('u_View', scale(translate(identity(), [0,-0.25,0]), [1,1,1]));
quad.setInt('u_UseTexture', false);
quad.draw([
    -0.5, 0.5, 0,
    -0.5, -0.5, 0,
    0.5, 0.5, 0,
    0.5, -0.5, 0,
], [0,0,0,0,0,0,0,0], [
    1,0,0,1,
    0,1,0,1,
    0,0,1,1,
    1,1,1,1
]);
const texture = loadTexture(".\\images\\background.png");
let interval, timeout;
interval = setInterval(() => {
    if (!texture.loaded) return;
    clear();
    quad.setInt('u_UseTexture', true);
    quad.setTexture('u_Texture', texture);
    quad.draw([
        -0.5, 0.5, 0,
        -0.5, -0.5, 0,
        0.5, 0.5, 0,
        0.5, -0.5, 0,
    ], [
        0,0,
        0,1,
        1,0,
        1,1
    ], [
        1,.5,.5,1,
        .5,1,.5,1,
        .5,.5,1,1,
        1,1,1,1
    ]);
    clearInterval(interval);
    clearTimeout(timeout);
}, 200);
timeout = setTimeout(() => {
    console.error("Failed to load texture.");
    clearInterval(interval);
}, 10000);

/*const inputFile = document.createElement("input");
inputFile.type = "file";
inputFile.id = "inputssbp";
inputFile.accept = ".ssbp";
inputFile.onchange = function(event) {
    if (event.target.files.length > 0)
        loadSsbp(event.target.files[0])
            .then(print)
            .catch(print);
};
document.getElementById("miniunit").appendChild(inputFile);*/

