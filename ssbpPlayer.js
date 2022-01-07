import {loadSsbp} from './loadSsbp.js';
import {print} from './printer.js';

const inputFile = document.createElement("input");
inputFile.type = "file";
inputFile.id = "inputssbp";
inputFile.accept = ".ssbp";
inputFile.onchange = function(event) {
    if (event.target.files.length > 0)
        loadSsbp(event.target.files[0])
            .then(print)
            .catch(print);
};
document.getElementById("miniunit").appendChild(inputFile);
