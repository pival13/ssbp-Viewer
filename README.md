# ssbp-Viewer
Used for viewing ssbp version 3 animation files

## Compilation

<i>**Note**: Precompiled versions are available under the `bin` folder. Windows only. Required x64 ImageMagick's DLLs at 8 bits-per-pixels components to run ([ImageMagick-XXX-Q8-x64-dll.exe](https://imagemagick.org/script/download.php#windows))</i>

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

**glad** source files are also necessary in order to launch OpenGL. They are already provided, but you may have to update them in order to match your devices. Look at the header of `include/glad/glad.h` for more informations.

### Build

Use the following commands in order to build the project:

```bash
cmake -S . -B build
cmake --build build --config Release
```

If everything worked smoothly, two binaries named `SsbpPlayer` and `SsbpSaver` should be visible on the `bin` folder at the root of the project.

## How to use

### SsbpPlayer

SsbpPlayer is a small program that recreate a SSBP animation when given the corresponding .ssbp file. It has been mainly build for, and tested with, the animations from the Fire Emblem Heroes mobile game. SSBP animations from other sources may or may not work.

Loading an animation can either be done at start time, when launching the program from a terminal, or at runtime, after launching the program, whenever you are asked to enter an ssbp file. On both cases, the same arguments can be specify, following the same rules. More informations about those arguments are available on the [Common arguments](#common-arguments) section.

Once an animation file have been loaded, several interactions are available.
* Pausing / Resuming the animation by pressing the `space` bar.
* Changing the current frame by pressing the `left` and `right` arrows, or `A` and `D` keys. Note that this will paused the animation.
* Changing the current animation by pressing the `up` and `down` arrows, or the `W` and `S` keys. Note that this will resume the animation if it has been paused.
* Enabling / Disabling looping animation by pressing the `L` key.
* Changing the animation speed by pressing the `1`, `2` and `3` keys.
* * 1 will reduce the speed by 0.1, up to -2.
* * 2 will reset the speed at 1.
* * 3 will increase the speed by 0.1, up to 2.
* Vertically flipping the animation by pressing the `X` key. The animation will be mirrored relatively to the Y axis, centered on its pivot point.
* Moving the animation by dragging it with your cursor.
* Zooming and dezooming by scrolling or dragging with the right button along the Y axis.
* Resetting the position and zoom level of the animation by pressing the `C` key.
* Saving the current frame by pressing the `Q` key.
* Saving the current animation by pressing the `E` key. Note that this will change the currently played frame.
* Showing the help by pressing the `H` key.

Note the following about saved images:
* Single frame are always saved as PNG without background, even if a background is specify.
* Whole animations are always saved as GIF with background, using a white background by default.
* Single frames are cropped in order to keep only the animation.
* Whole animations are of the size of the windows. The bigger the windows, the longer it will take to saved the animation.
* A delay may occured between the moment an animation is played, and the moment the file is created. A message will appeared whenever the process is over.
* Whole animations are always looped. However when the looping option is disable (with the L key), the first frame is displayed for half a second, while the last frame is kept a whole second, resulting on a effective break.
* Animations are saved as appeared on the windows. If it is cropped, moved or scaled, so will it be the saved file.
* Every file are saved on the `Screenshot` folder under the current working directory, creating it if necessary. Look at the ingame message for the subfolder and file's names.

### SsbpSaver

SsbpSaver is a small program that will save some images from a given SSBP animation. It has been mainly build for units' animations of the Fire Emblem Heroes mobile game. Using it with any other SSBP will very likely produce nothing at all.

Loading an animation must be done at runtime, after launching the program. No message are displayed, and each line is interpreted as a different command. Closing the program require entering `Ctrl+Z`. It used the same arguments as `SsbpPlayer`, which are described on the [Common arguments](#common-arguments) section.

SsbpSaver will only save animations from the `body_anim` animation pack. Each of its animation will be saved as GIF, plus a particular frame from the following ones as PNG: `Idle`, `Ok`, `Ready`, `Jump`, `Attack1`, `Attack2`, `AttackF`, `Damage`, `Pairpose`, `Cheer`, `Transform`.

Unlike SsbpPlayer, both PNG and GIF are cropped to saved only the given animation. Plus, there is no default background when saving as a GIF.

SsbpSaver can be compiled with or without `SAVE_SPRITE` and `SAVE_ANIM` macros to enable/disable the creation of PNG and GIF respectively.

### Common arguments

Both `SsbpPlayer` and `SsbpSaver` support the same arguments, which produce the same result. These are detailled below:

| Short option | Long option | Description |
|--------|--------|---------|
| `-w <width>`<br>`-h <height>` | `--width=<width>`<br>`--height=<height>` | Set the initial width and height of the windows. Defaults to `500x500`.
| `-p <posX>x<posY>{px/%}` | `--position=<posX>x<posY>{px/%}` | Set the initial position of the pivot point of the animation.<br>One of `px` or `%` must be specified. The point `0x0` correspond to the left bottom corner. Defaults to `50x25%`.<br>Must be specify after any `-w` and `-h`.
|||
| `-bg <background image>` | `--background=<background image>` | Path to the background image to be used. Must be specify before any of the upcoming options, or they will be lost.
| `-f`<br>`-fh`<br>`-fw`<br>`-s`<br>`-o`<br><br><br> | `--fit`<br>`--fitHeight`<br>`--fitWidth`<br>`--stretch`<br>`--original`<br>`--scale=<coef>`, `--scale=<coeffX>x<coeffY>`<br>`--size=<sizeX>x<sizeY>` | Set the behaviour of the background image.<br>`o`/`original` keep the initial size of the background.<br>`f`/`fit` stretch the image to fully fit on the windows.<br>`fh`/`fitHeight` and `fw`/`fitWidth` scale the image to fully covered respectively the height and width of the windows.<br>`s`/`strecth` use either `fitHeight` or `fitWidth` according to the size of the windows.<br>`scale` scale the background by `coeffX` on X and `coeffY` on Y, or `coeff` for both X and Y.<br>`size` set the size of the background at `sizeX` x `sizeY` px.<br>Defaults to `fit` with SsbpPlayer and `original` with SsbpSaver.
|  | `--shift=<shiftX>x<shiftY>{px/%}` | Shift the background by `shiftX` pixel/percent on the X axis, and `shiftY` on the Y axis. Positive `shiftX` move the background to the right, while positive `shiftY` move to the top.

It is also possible to override some parts using on the following options:
* `~<textureName>,<newTexturePath>`: Override the texture `textureName` with the texture located at `newTexturePath`. Any part on the animation using `textureName` will now use the new one. Providing an invalid `newTexturePath` will result on hiding all parts using it.
* `~<partName>,<newTexturePath>`: Same as above, but the name specify is the name of one of the using the desired texture. It is currently impossible to only change a part, without changing those who shared its texture.
* `~<partName>,<ssbpPath>,<animationName>`: Override the part `partName` with the animation `animationName` from the SSBP animation file located at `ssbpPath`. 

Note that it is currently not possible to override a texture from a sub SSBP.

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