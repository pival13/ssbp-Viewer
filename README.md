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
  * `partName:imagePath`: Change the sprite used on the part `partName` by the corresponding sprite on the image at `imagePath`. Images must be on PNG or WEBP format.
  * `partName:ssbp:animeName`: Force the part `partName` to use the animation `animeName` describe on the ssbp file at `ssbp`. `ssbp` can be the path to the file, or its name (without extension) if it as already been described. If omitted, `ssbp` default to the current ssbp file.
* `-bg arg`, `--background=arg`: Change the image used as background. Defaults to a `background.png` file on the binary folder. If the image is not loaded, no background are used.
  * The behaviour of the background can be changed by editing the `background.vert` file, situated on the `shaders` folder at the root of the program's folder. More instructions are available there.
* `-p arg`, `--position=arg`: Set the initial position of the element to draw. The position must be of format: `posX,posY`, where `posX` and `posY` can be any real, optionnaly followed by `px` or `%`. 0 is meant for the bottom and left side, while 1 is used for the top and right side. Defaults to `0.5,0.25`.

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

When saving an animation, the image is cropped to keep only the animation. Moreover, single frame are saved as PNG without background, while animation are saved as GIF with background, if available. The scale is kept and the position may lead to cropped image.<br>
Screenshot are always saved on a `Screenshot` folder under the current working directory.

### Example

```bash
# Basic command, Windows
bin\ssbpViewer.exe images\ch00_00_Eclat_X_Avatar00_blow\ch00_00_Eclat_X_Avatar00_blow.ssbp

# Weapon overload, Linux
# Bind option is split on first and last colon (:), enabling the use of Windows absolute path
# No spaces must be present around colon, unlike here
bin/ssbpViewer \
    ~/FehAssets/Common/Unit/ch00_03_Anna_F_Normal/ch00_03_Anna_F_Normal.ssbp \
    -b \
        Wep_BaseR:\
        ~/FehAssets/Common/Wep/wep_ax009.ssbp:\
        wep_ax009/Wep_Normal

bin/ssbpViewer \
    ~/FehAssets/Common/Unit/ch00_03_Anna_F_Normal/ch00_03_Anna_F_Normal.ssbp \
    -b \
        Wep_BaseR:\
        ~/FehAssets/Common/Wep/wep_ax002.png

# Archer example, Windows
# Archer's weapon are splitted as bow (wep_bw*) and arrow (wep_ar*)
# Each of these must be set to the correct hand
bin\ssbpViewer.exe
  -b     Wep_BaseL:C:\{...}\FehAssets\Common\Wep\wep_bw053_up.ssbp:wep_bw053_up/Wep_Normal
  --bind=Wep_BaseR:C:\{...}\FehAssets\Common\Wep\wep_ar006_up.ssbp:wep_ar006_up/Wep_Normal
  C:\{...}\FehAssets\Common\Unit\ch01_28_Kleine_F_Normal\ch01_28_Kleine_F_Normal.ssbp

# The same ssbp doesn't need to be loaded twice
bin/ssbpViewer \
    ~/FehAssets/Common/Unit/ch11_16_Ortina_FF_Pair/ch11_16_Ortina_FF_Pair.ssbp \
    -b Wep_BaseR:~/FehAssets/Common/Wep/wep_ac009.ssbp:wep_ac009/Wep_Normal \
    -b Wep_BaseL:wep_ac009:wep_ac009/Wep_Normal
```

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
  * ~~Weapon~~
  * Effect
  * Accessory
* Support background
  * ~~Add background~~
    * ~~Change background on command-line or runtime~~
  * ~~Add background option~~
    * Change background option on command-line or runtime
  * Add foreground
    * Change foreground on command-line or runtime