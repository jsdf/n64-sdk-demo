# Nintendo 64 homebrew demo & tutorial

This is a demo app with heavily commented source showing basic usage of the N64 SDK and the NuSystem framework. The game-specific code is in [stage00.c](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c).

The compiled rom file can be found [here](https://github.com/jsdf/n64-sdk-demo/raw/master/squaresdemo.n64.zip). It can be run with an emulator or an N64 flashcart. Try pressing the A and B buttons.

![recording of the demo](https://media.giphy.com/media/hTmlPHZq8LSDnyGRH7/giphy.gif)

## How to build (macOS or linux):

- Install [wine](https://www.winehq.org/)
- Install the n64 sdk. this repo assumes it's in the default location of `C:\ultra` (in the wine filesystem). If you've installed it somewhere else, you'll need to update this path in `compile.bat`
- Edit `./build.sh` so that WINE_PATH points to your wine binary
- Run `./build.sh`. This should build squaresdemo.n64
- Run squaresdemo.n64 with an emulator or an N64 flashcart

## Understanding the code (or, an N64 homebrew tutorial)

There are basically two options for making an N64 game these days:
- the official Nintendo SDK from the 90s, which comes in Windows 95 and SGI IRIX flavours.
- the modern open source toolchain, which centers on the [libdragon](https://github.com/DragonMinded/libdragon) library and tools.

This tutorial uses the SDK. RetroReversing has a [pretty good tutorial for installing and using the SDK](https://www.retroreversing.com/n64-sdk-setup) under Wine, which will allow you to compile the code in this repo.

The N64 SDK comes with a small framework for quickly starting a new game, called NuSystem. The N64 comes with an OS (really, a library that you link into your game binary and boot on the bare metal), which provides features like threads and I/O, but still requires a fair bit of boilerplate to get a game engine set up. NuSystem removes the need to think about threads and initializing the hardware, and just lets you provide the typical `setup()`, `update()`, and `draw()` callbacks that form the core of many simple game engines. In NuSystem these functions are typically called [`initStage00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L41), [`updateGame00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L60), and [`makeDL00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L91) respectively, where `00` is the stage/level number of the game.

### Initialization
When a game using NuSystem boots, it runs the `mainproc()` function in [main.c](https://github.com/jsdf/n64-sdk-demo/blob/master/main.c#L8). This then calls [`initStage00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L41) to initialize the level, and then [registers the `stage00` callback](https://github.com/jsdf/n64-sdk-demo/blob/master/main.c#L20) so that [`updateGame00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L60), and [`makeDL00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L91) can be called on each frame.

From there it's up to you to fill in the logic for these functions with your game-specific code.

### Reading controller input and updating the game world

In [`initStage00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L41), we initialized some state: the `squaresRotationDirection` boolean, and the `squaresRotations` array.

In [`updateGame00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L60) we first read the controller input, then update these values as appropriate, each frame. In this example we rotate all the squares a bit each frame, and if the A button is pressed, reverse the direction of their rotation. We'll use these values to determine what to render in the [`makeDL00()`](https://github.com/jsdf/n64-sdk-demo/blob/master/stage00.c#L91) function.

### Rendering

On the N64, at a high level the procedure for rendering graphics looks something like this:

- start a new a 'display list' (a list of commands to draw stuff on screen, that is stored in memory)
- calculate projection and modelview matrices based on camera position and properties (type of projection, field of view, aspect ratio)
- build up the display list by iterating through world objects and adding commands to transform the current drawing position/rotation/scale using transformation matrices, and then draw model meshes, potentially with some texture and lighting settings
- send the display list off as a 'graphics task' to the RCP ('Reality Co-Processor', basically the GPU of the N64) to be rendered

Rendering on the N64 is a cooperative act between the main CPU, which runs the game logic and produces the list of things to draw, and the RCP, which performs the drawing of one frame while the main CPU moves on to running the logic and producing the display list for the next frame. To do this, we will need to allocate some structures to hold the displaylists and other data that will be shared between the code which runs on the main CPU (our program) and the code which runs on the RCP (called 'microcode', provided in binary form as part of the SDK). We need to allocate separate instances of this shared data for each frame, and switch which instance we're using when starting a new frame, like a circular buffer. The `gfxTaskSwitch()` function in [graphic.c](https://github.com/jsdf/n64-sdk-demo/blob/master/graphic.c#L14-L22) shows one implementation of this. For a more detailed picture of the relationship between the CPU and the RCP, see [this diagram](https://github.com/jsdf/n64-sdk-demo/blob/master/n64%20cpu%20rcp.png).

#### The Matrix Stack

Another concept that's worth understanding is the 'matrix stack'. This concept comes directly from OpenGL 1.0 (which makes sense as both the Nintendo 64 hardware and OpenGL were developed by SGI). If you're put off by matrix math, don't worry, you don't need to understand it to make use of this feature. If you've used 2D drawing APIs like Turtle graphics or the Canvas2DContext in the HTML Canvas API, you'll be familiar with moving the drawing position by applying successive relative transformations such as translation, rotation and scaling, and this is very similar.

The matrix stack is a stack data structure of matrices representing transformations of current drawing position in 3D space, which can be pushed onto and popped off. Pushing a matrix onto the stack effectively means applying a relative transformation to the drawing position which we can later undo by popping it back off. This allows rendering of hierarchical structures which might have transformations relative to some parent object, such as the positions and rotations of the wheels of a car relative to the car, and the car itself positioned and rotated in relation to the world. Modern OpenGL no longer includes this concept in its API, but for the N64 being able to offload the work of performing the matrix math (to transform objects in 3D space) to the RCP necessitated this sort of API.

In OpenGL 1.0 this looks something like:

```c

glMatrixMode(GL_MODELVIEW); // using modelview matrix stack
glLoadIdentity(); // start at 0,0,0, with no rotation or scaling
glPushMatrix();
  glTranslate(10, 0, 0); // translate in the x axis
  drawCube(); // draw something, translated
  glPushMatrix();
    glRotate(0, 45, 0); // rotate around the y axis
    glScale(1, 2, 1); // scale in the y axis
    drawPyramid(); // draw something else, translated, rotated and scaled
  glPopMatrix(); // undo rotation and scale
glPopMatrix(); // undo translation
drawSphere(); // draw something else, back at 0,0,0
```

On the N64, the equivalent code is:

```c
int curMatrix = 0;

// start at 0,0,0, with no rotation or scaling
guMtxIdent(&gfxTask->objectTransforms[curMatrix]);
gSPMatrix(displayListPtr++,
  OS_K0_TO_PHYSICAL(&(gfxTask->objectTransforms[curMatrix++])),
  G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_LOAD
);
// translate in the x axis
guTranslate(&gfxTask->objectTransforms[curMatrix], 10, 0, 0); 
// push translation matrix
gSPMatrix(displayListPtr++,
  OS_K0_TO_PHYSICAL(&(gfxTask->objectTransforms[curMatrix++])),
  G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL
);

drawCube(); // draw something, translated

// rotate around the y axis
guRotate(&gfxTask->objectTransforms[curMatrix], 0, 45, 0); 
// scale in the y axis
guScale(&gfxTask->objectTransforms[curMatrix], 1, 2, 1); 

// push transformation matrix
gSPMatrix(displayListPtr++,
  OS_K0_TO_PHYSICAL(&(gfxTask->objectTransforms[curMatrix++])),
  G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL
);

drawPyramid(); // draw something else, translated, rotated and scaled

// undo rotation and scale
gSPPopMatrix(displayListPtr++, G_MTX_MODELVIEW);
// undo translation
gSPPopMatrix(displayListPtr++, G_MTX_MODELVIEW);

drawSphere(); // draw something else, back at 0,0,0
```

Although there is a strong correspondence between the two APIs, there are some important differences. On N64, you are responsible for allocating and the matrix (`Mtx`) data structures, whereas OpenGL hides them inside its implementation. Additionally, when you use a function like `guTranslate()` or  `guRotate()`, it immediately mutates the matrix you passed in. However, rendering happens later (on the RCP), so any drawing commands between the surrounding `gSPMatrix()` and `gSPPopMatrix()` commands will be rendered with the same, final matrix value (eg. the same transform).


#### Rendering squares

The rendering of each square is wrapped up in the `drawSquare()` function. The square is defined in `squareVerts`, and it is made up of 2 triangles, and colored by RGB colors defined for each vertex.
For each of the items in the `squares` array, we push a transformation matrix onto the matrix stack with the position and current rotation of the square, then add a bunch of displaylist commands to load the triangle vertices forming the square, and set a bunch of RCP settings affecting the appearance of the square.  Finally we render the square with the `gSP2Triangles()` command. We also add a `gDPPipeSync()` command to mark the end of the current primative (rendered object sharing the same settings), and use pop our transformation back off the stack with `gSPPopMatrix()`.

#### Rendering a complex mesh

A common approach to including models in your game created in a 3d modeling is to convert them into C source files containing static display lists. A static displaylist is a list of displaylist commands defined in an array. You can find an example of this in the `drawN64Logo()` function, which uses the `gSPDisplayList()` command to render the static display lists defined in [n64logo.h](https://github.com/jsdf/n64-sdk-demo/blob/master/n64logo.h#L1).

