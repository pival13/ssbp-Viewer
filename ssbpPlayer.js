import {loadSsbp} from './loadSsbp.js';
import {print} from './printer.js';
import {initialize as initWebGL, quad} from './webgl.js';

console.log(quad)

const canvas = document.getElementById("drawer");

initWebGL(canvas);

quad.draw([
    -0.5, 0.5, 0,
    -0.5, -0.5, 0,
    0.5, 0.5, 0,
    0.5, -0.5, 0,
], [0,0,0,0,0,0,0,0], [
    1,0,0,1,
    0,1,0,1,
    0,0,1,1,
    0,0,0,1
]);

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

