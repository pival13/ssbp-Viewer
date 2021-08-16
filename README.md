# ssbp-Viewer
Used for viewing ssbp version 3 animation files

## Compilation

<i>**Note**: A compiled version is available on the `bin` folder. Windows only.</i>

### Prerequisite

This project use CMake to compile and build the program. See [its website](https://cmake.org/install/) for more information about its installation.

The following package are used by CMake in order to build the project. It is recommanded to download them or build them from source.

* ImageMagick ([Download](https://imagemagick.org/script/download.php), [GitHub repo](https://github.com/ImageMagick/ImageMagick))
* glfw ([Download](https://www.glfw.org/download.html), [GitHub repo](https://github.com/glfw/glfw))
* glm ([GitHub repo](https://github.com/g-truc/glm))

You can typically build them with CMake, by following those commands:

```bash
cd [folder]
cmake -S . -B build
cmake --build build
sudo cmake --install build # On Windows run the terminal as Administrator
```

**glad** source files are also necessary in order to launch OpenGL. They can be auto-generated on the [glad website](https://glad.dav1d.de). We are using **OpenGL** with **Core** profil for **C/C++**, with a gl version of at least **3.0**. Once generated, the file `glad.c` must be put on the `src` folder, and `glad.h` on `include/glad`.

### Build

Use the following commands in order to build the project:

```bash
cmake -S . -B build
cmake --build build --config Release
```

If everything worked smoothly, a binary named `ssbpViewer` should be visible on the `bin` folder at the root of the project.

## How to use

SsbpViewer is a small program that recreate a ssbp animation when given the corresponding .ssbp file. It has been mainly build for and test with the animation from the Fire Emblem Heroes mobile game. Ssbp animation from other sources may or may not work.

Loading an animation can either be done at start time, by specifying the path to the ssbp file, or at run time by dropping an ssbp file and pressing enter.

SsbpViewer also support the following options:
* `-b arg`, `--bind=arg`: Change the content of a part, depending on the format of `arg`:
* * `partName:imagePath`: Change the sprite used on the part `partName` by the corresponding sprite on the image at `imagePath`. Images must be on PNG or WEBP format.
* * `partName:ssbp:animeName`: Force the part `partName` to use the animation `animeName` describe on the ssbp file at `ssbp`. `ssbp` can be the path to the file, or its name (without extension) if it as already been described. If omitted, `ssbp` default to the current ssbp file.

Once an animation file have been loaded several interactions are available.
* Change the animation played: right and left arrows, or the A and D keys.
* Change the animation speed: 1 to slow down and 2 to speed up. 3 to reset.
* Pause/Resume the animation (when looping): space bar.
* Restart the animation (when not looping): space bar.
* Change frame: up and down arrows, or W and S keys.
* Enable/Disable looping animation: L key.
* Vertically flip the animation: X key.
* Zoom and move animation: mouse scrolling and draging.
* Reset animation scale and position: C key.
* Save animation: Q for a single frame, E for the whole animation.
* Show help: H key.

SsbpViewer is able to load a background to be displayed behind the animation.<br>
Currently, if there is a file named `background.png` on an `images` folder at the root of the program's folder, this image will be used as the background.<br>
The behaviour of the background can be changed by editing the `background.vert` file, situated on the `shaders` folder at the root of the program's folder. More instructions are available there.

When saving an animation, the image is cropped to keep only the animation. Moreover, single frame are saved as PNG without background, while animation are saved as GIF with background, if available. The scale is kept and the position may lead to cropped image.<br>
Screenshot are always saved on a `Screenshot` folder under the current working directory.

## TODO

* Fix display error (e.g. Transformed F!Edelgard "Wings", Transformed Nifl Jump)
* ~~Use premultiplied alpha on image~~
  * Premultiply alpha on shader instead of texture loading ?
* Support blending
  * ~~Mix~~
  * ~~Add~~
  * Substruct
  * Mult
* Support additional elements
  * Weapon
  * Effect
  * Accessory
* Support background
  * ~~Add background~~
    * Change background on command-line or runtime
  * ~~Add background option~~
    * Change background option on command-line or runtime
  * Add foreground
    * Change background on command-line or runtime