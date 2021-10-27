import {loadSsbp} from './loadSsbp.js';
import {print} from './printer.js';
import {initialize as initWebGL, quad, clear, setTextureBaseDir, getTexture} from './webgl.js';
import {translate, scale, identity} from './math.js';
import { drawAnimation } from './ssbpPlayer.js';

const canvas = document.getElementById("drawer");

initWebGL(canvas);

quad.setMat4('u_View', scale(translate(identity(), [0,0,0]), [2/640,2/480,1]));
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
let interval, timeout;
interval = setInterval(async () => {
    const texture = await getTexture({name: "background", path: "./images/background.png"});
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

const inputFile = document.createElement("input");
inputFile.type = "file";
inputFile.id = "inputssbp";
inputFile.accept = ".ssbp";
inputFile.onchange = function(event) {
    if (event.target.files.length > 0)
        loadSsbp(event.target.files[0])
            .then(ssbp => {
                setTextureBaseDir(event.target.value.substring(0, event.target.value.replace(/\\/g,"/").lastIndexOf("/")))
                let i = 0;
                const inter = setInterval(() => {
                    clear();
                    drawAnimation(ssbp, ssbp.animationPacks[0].name, ssbp.animationPacks[0].animations[0].name, i++, identity())
                        .catch(console.log);
                }, 1000);
            }).catch(err => {print(err);console.error(err)});
};
document.getElementById("miniunit").appendChild(inputFile);

