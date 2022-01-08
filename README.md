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

If everything worked smoothly, a binary named `ssbpPlayer` should be visible on the `bin` folder at the root of the project.

## How to use

SsbpPlayer is a small program that recreate a ssbp animation when given the corresponding .ssbp file. It has been mainly build for and test with the animation from the Fire Emblem Heroes mobile game. Ssbp animation from other sources may or may not work.

Loading an animation can either be done at start time, by specifying the path to the ssbp file, or at run time by dropping an ssbp file and pressing enter.

SsbpPlayer also support the following options at start time:
* `~name,texture`, `~name,ssbp,animation`: Change the content display for parts, or whole texture file. The SSBP file must be loaded first, or the change will be lost. The arguments have the following meanings:
  * `name`: The name of the texture or part to replace.
  * `texture`: The path to a PNG or WEBP image.
  * `ssbp`: The path to a SSBP file.
  * `animation`: The name of the animation to be used.
* `-bg arg`, `--background=arg`: Change the image used as background. Defaults to a white background.
* `-w width`, `--width=width`, `-h height`, `--height=height`: Set the initial size of the window.
<!--* `-p arg`, `--position=arg`: Set the initial position of the element to draw. The position must be of format: `posX,posY`, where `posX` and `posY` can be any real, optionnaly followed by `px` or `%`. 0 is meant for the bottom and left side, while 1 is used for the top and right side. Defaults to `0.5,0.25`.-->

Once an animation file have been loaded several interactions are available.
* Change frame: right <code>$\rightarrow$</code> and left <code>$\leftarrow$</code> arrows, or `A` and `D` keys.
* Change the animation played: up <code>$\uparrow$</code> and down <code>$\downarrow$</code> arrows, or the `W` and `S` keys.
* Pause/Resume the animation: `Space bar`.
* Enable/Disable looping animation: `L` key.
* Change the animation speed: `1` to slow down and `3` to speed up. `2` to reset.
* Vertically flip the animation: `X` key.
* Zoom and move animation: mouse scrolling <code>$\downdownarrows$</code> and draging <code>$\mapsto$</code>.
* Reset scale and position: `C` key.
* Save animation: `Q` for a single frame, `E` for the whole animation.
* Show help: `H` key.

By default, single frames are saved as PNG without background, while animations are saved as GIF with the set–or white–background. However, if the `Control` key is held down when pressing the corresponding key, this behaviour change. Single frames are then saved with the set background–or a white background if unavailable. Animations, on the other hand, are saved as multiple PNGs with the set background, or without background if none was set.<br>
When saving images, whether animations or frames, these are cropped to keep only the animation. However, having the `Caps Lock` key on disable the crop completely, resulting on an image of the size of the window. Note that this may slow down the creation of the file.<br>
Screenshot are always saved on a `"/Screenshot/"` folder under the current working directory.

### Example

```bash
# Basic command, Windows
bin\ssbpPlayer.exe images\ch00_00_Eclat_X_Avatar00_blow\ch00_00_Eclat_X_Avatar00_blow.ssbp

# Weapon overload, Linux
# Bind option is split on first and last colon (:), enabling the use of Windows absolute path
# No spaces must be present around colon, unlike here
bin/ssbpPlayer \
    ./FehAssets/Common/Unit/ch00_03_Anna_F_Normal/ch00_03_Anna_F_Normal.ssbp \
    ~ \
        Wep_BaseR,\
        ./FehAssets/Common/Wep/wep_ax009.ssbp,\
        wep_ax009/Wep_Normal

bin/ssbpPlayer \
    ./FehAssets/Common/Unit/ch00_03_Anna_F_Normal/ch00_03_Anna_F_Normal.ssbp \
    ~ \
        wep_ax,\
        ./FehAssets/Common/Wep/wep_ax002.png

# Archer example, Windows
# Archer's weapon are splitted as bow (wep_bw*) and arrow (wep_ar*)
# Each of these must be set to the correct hand
bin\ssbpPlayer.exe
  C:\{...}\FehAssets\Common\Unit\ch01_28_Kleine_F_Normal\ch01_28_Kleine_F_Normal.ssbp
  ~Wep_BaseL:C:\{...}\FehAssets\Common\Wep\wep_bw053_up.ssbp:wep_bw053_up/Wep_Normal
  ~Wep_BaseR:C:\{...}\FehAssets\Common\Wep\wep_ar006_up.ssbp:wep_ar006_up/Wep_Normal
```

## TODO

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
    * Change background on command-line or runtime
  * Add background option
    * Change background option on command-line or runtime
  * Add foreground
    * Change foreground on command-line or runtime