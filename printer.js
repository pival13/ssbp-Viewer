export function print(text) {
    document.getElementById("placeToPrint").innerHTML += text + ": <pre>" + JSON.stringify(text, null, 2) + "</pre><br>";
};