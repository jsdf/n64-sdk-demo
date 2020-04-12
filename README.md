# n64 homebrew demo

this is a demo app with heavily commented source showing basic usage of the n64 sdk

the compiled rom file can be found [here](https://github.com/jsdf/squaresdemo/raw/master/squaresdemo.n64)

![recording of the demo](https://media.giphy.com/media/J6V3fgMbTGQdqGFwv9/giphy.gif)

## to build (macOS or linux):

- install [wine](https://www.winehq.org/)
- install the n64 sdk. `compile.bat` in this repo assumes it's in the default location of `C:\ultra`
- edit `./build.sh` so that WINE_PATH points to your wine binary
- run `./build.sh`. this should build squaresdemo.n64
- run squaresdemo.n64 with an emulator or an N64 flashcart

## N64 homebrew tutorial

There are basically two options for making an N64 game these days:
- the official Nintendo SDK from the 90s, which comes in Windows 95 and SGI IRIX flavours. Using this software is legally sketchy, but it's very old and I don't think there's much harm in hobbyists making non-commerical games with it. You can find it on [ultra64.ca](https://ultra64.ca/).
- the modern open source toolchain, which centers on the [libdragon](https://github.com/DragonMinded/libdragon) library and tools. The open source toolchain has come a long way, but at this stage it is still more low-level than the Nintendo SDK, and is especially limited in the area of 3D rendering.

This tutorial uses the SDK. RetroReversing has a [pretty good tutorial for installing and using the SDK](https://www.retroreversing.com/n64-sdk-setup) under Wine. Interestingly it is probably easier to run on macOS or Linux using Wine than on a modern version of Windows, probably owing to the fact that this is Windows 95-era software. You can also find a lot of useful information on the [N64Brew Discord](https://discord.gg/KERXwaT).

The N64 SDK comes with a small framework for quickly starting a new game, called NuSystem. The N64 comes with an OS (really, a library that you link into your game binary and boot on the bare metal), which provides features like threads and I/O, but still requires a fair bit of boilerplate to get a game engine set up. NuSystem removes the need to think about threads and initializing the hardware, and just lets you provide the typical init(), update(), and draw() callbacks that form the core of many simple game engines. In NuSystem these functions are typically called [`initStage00()`](https://github.com/jsdf/squaresdemo/blob/master/stage00.c#L31-L43), [`updateGame00()`](https://github.com/jsdf/squaresdemo/blob/master/stage00.c#L192-L207), and [`makeDL00()`](https://github.com/jsdf/squaresdemo/blob/master/stage00.c#L46-L130) respectively, where `00` is the stage/level number of the game.

### Rendering

On the N64, at a high level the procedure for rendering graphics looks something like this:

- start a new a 'display list' (a list of commands to draw stuff on screen, that is stored in memory)
- calculate projection and modelview matrices based on camera position and properties (type of projection, field of view, aspect ratio)
- build up the display list by iterating through world objects and adding commands to transform the current drawing position/rotation/scale using transformation matrices, and then draw model meshes, potentially with some texture and lighting settings
- send the display list off as a 'graphics task' to the RCP ('Reality Co-Processor', basically the GPU of the N64) to be rendered

Rendering on the N64 is a cooperative act between the main CPU, which runs the game logic and produces the list of things to draw, and the RCP, which performs the drawing of one frame while the main CPU moves on to running the logic and producing the display list for the next frame. To do this, we will need to allocate some structures to hold the displaylists and other data that will be shared between the code which runs on the main CPU (our program) and the code which runs on the RCP (called 'microcode', provided in binary form as part of the SDK). We need to allocate separate instances of this shared data for each frame, and switch which instance we're using when starting a new frame, like a circular buffer.

The `gfxTaskSwitch()` function in [graphic.c](https://github.com/jsdf/squaresdemo/blob/master/graphic.c#L14-L22) shows one implementation of this.


### The Matrix Stack

Another concept that's worth understanding is the 'matrix stack'. This concept comes directly from OpenGL 1.0 (which makes sense as both the Nintendo 64 hardware and OpenGL were developed by SGI). If you're put off by matrix math, don't worry, you don't need to understand it to make use of this feature. If you've used 2D drawing APIs like Turtle graphics or the Canvas2DContext in the HTML Canvas API, you'll be familiar with moving the drawing position by applying successive relative transformations such as translation, rotation and scaling, and this is very similar. The matrix stack is a stack data structure of matrices representing transformations of current drawing position in 3D space, which can be pushed onto and popped off. Pushing a matrix onto the stack effectively means applying a relative transformation to the drawing position which we can later undo by popping it back off. This allows rendering of hierarchical structures which might have transformations relative to some parent object, such as the positions and rotations of the wheels of a car relative to the car, and the car itself positioned and rotated in relation to the world. Modern OpenGL no longer includes this concept in its API, but for the N64 being able to offload the work of performing the matrix math (to transform objects in 3D space) to the RCP necessitated this sort of API.


