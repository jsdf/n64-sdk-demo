
#include <assert.h>
#include <nusys.h> 

#include "graphic.h"
#include "main.h"
#include "stage00.h"


Vec3d cameraPos = {-200.0f, -200.0f, -200.0f};
Vec3d cameraTarget = {0.0f, 0.0f, 0.0f};
Vec3d cameraUp = {0.0f, 1.0f, 0.0f};

// the positions of the squares we're gonna draw
#define NUM_SQUARES 5
struct Vec3d squares[NUM_SQUARES] = {
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 200.0f},
  {0.0f, 0.0f, -100.0f},
};

int squaresRotations[NUM_SQUARES] = {
  0,
  20,
  40,
  40,
  40,
};

int squaresRotationDirection = 0;

// the 'draw' function
void makeDL00() {
  // in the older version of C used by the N64 compiler (roughly C89), variables
  // must be declared at the top of a function or block scope
  unsigned short perspNorm;
  Gfx * displayListStart;
  GraphicsTask * gfxTask;
  
  // also updates the displayListPtr global variable
  gfxTask = gfxSwitchTask();
  // keep track of start
  displayListStart = displayListPtr;


  // prepare the RCP for rendering a graphics task
  gfxRCPInit();

  // clear the color framebuffer and Z-buffer, similar to glClear()
  gfxClearCfb();
 
  // initialize the projection matrix, similar to glPerspective() or glm::perspective()
  guPerspective(&gfxTask->projection, &perspNorm, FOVY, ASPECT, NEAR_PLANE,
                FAR_PLANE, 1.0);

  // Our first actual displaylist command. This writes the command as a value at
  // the tail of the current display list, and we increment the display list
  // tail pointer, ready for the next command to be written.
  // As for what this command does... it's just required when using a perspective
  // projection. Try pasting 'gSPPerspNormalize' into google if you want more
  // explanation, as all the SDK documentation has been placed online by
  // hobbyists and is well indexed.
  gSPPerspNormalize(displayListPtr++, perspNorm);

  // initialize the modelview matrix, similar to gluLookAt() or glm::lookAt()
  guLookAt(&gfxTask->modelview, cameraPos.x, cameraPos.y,
           cameraPos.z, cameraTarget.x, cameraTarget.y,
           cameraTarget.z, cameraUp.x, cameraUp.y, cameraUp.z);

  // load the projection matrix into the matrix stack.
  // given the combination of G_MTX_flags we provide, effectively this means
  // "replace the projection matrix with this new matrix"
  gSPMatrix(
    displayListPtr++,
    // we use the OS_K0_TO_PHYSICAL macro to convert the pointer to this matrix
    // into a 'physical' address as required by the RCP 
    OS_K0_TO_PHYSICAL(&(gfxTask->projection)),
    // these flags tell the graphics microcode what to do with this matrix
    // documented here: http://n64devkit.square7.ch/tutorial/graphics/1/1_3.htm
    G_MTX_PROJECTION | // using the projection matrix stack...
    G_MTX_LOAD | // don't multiply matrix by previously-top matrix in stack
    G_MTX_NOPUSH // don't push another matrix onto the stack before operation
  );
  gSPMatrix(displayListPtr++,
    OS_K0_TO_PHYSICAL(&(gfxTask->modelview)),
    // similarly this combination means "replace the modelview matrix with this new matrix"
    G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_LOAD
  );

  // we can use block scope to declare variables in the middle of a function
  {
    int i;
    for (i = 0; i < NUM_SQUARES; ++i)
    {
      drawSquare(gfxTask, i);
    }
  }

  // mark the end of the display list
  gDPFullSync(displayListPtr++);
  gSPEndDisplayList(displayListPtr++);

  // assert that the display list isn't longer than the memory allocated for it,
  // otherwise we would have corrupted memory when writing it.
  // isn't unsafe memory access fun?
  assert(displayListPtr - displayListStart < MAX_DISPLAY_LIST_COMMANDS);

  // create a graphics task to render this displaylist and send it to the RCP
  nuGfxTaskStart(
    displayListStart,
    (int)(displayListPtr - displayListStart) * sizeof (Gfx),
    NU_GFX_UCODE_F3DEX, // load the 'F3DEX' version graphics microcode, which runs on the RCP to process this display list
    NU_SC_SWAPBUFFER // tells NuSystem to immediately display the frame on screen after the RCP finishes rendering it
  );

}


// a static array of model vertex data
// this include the position (x,y,z), texture U,V coords (called S,T in the SDK docs)
// and vertex color values in r,g,b,a form
Vtx squareVerts[] = {
  //  x,   y,  z, flag, S, T,    r,    g,    b,    a
  { -64,  64, -5,    0, 0, 0, 0x00, 0xff, 0x00, 0xff  },
  {  64,  64, -5,    0, 0, 0, 0x00, 0x00, 0x00, 0xff  },
  {  64, -64, -5,    0, 0, 0, 0x00, 0x00, 0xff, 0xff  },
  { -64, -64, -5,    0, 0, 0, 0xff, 0x00, 0x00, 0xff  },
};

void drawSquare(GraphicsTask* gfxTask, int i) {
  Vec3d* square = &squares[i];
  // create a transformation matrix representing the position of the square
  guPosition(
    &gfxTask->objectTransforms[i],
    // rotation
    squaresRotations[i], // roll
    0.0f, // pitch
    0.0f, // heading
    1.0f, // scale
    // position
    square->x, square->y, square->z
  );

  // push relative transformation matrix
  gSPMatrix(displayListPtr++,
    OS_K0_TO_PHYSICAL(&(gfxTask->objectTransforms[i])),
    G_MTX_MODELVIEW | // operating on the modelview matrix stack...
    G_MTX_PUSH | // ...push another matrix onto the stack...
    G_MTX_MUL // ...which is multipled by previously-top matrix (eg. a relative transformation)
  );

  // load vertex data for the triangles
  gSPVertex(displayListPtr++, &(squareVerts[0]), 4, 0);
  // depending on which graphical features, the RDP might need to spend 1 or 2
  // cycles to render a primitive, and we need to tell it which to do
  gDPSetCycleType(displayListPtr++, G_CYC_1CYCLE);
  // use antialiasing, rendering an opaque surface
  gDPSetRenderMode(displayListPtr++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
  // reset any rendering flags set when drawing the previous primitive
  gSPClearGeometryMode(displayListPtr++,0xFFFFFFFF);
  // enable smooth (gourad) shading and z-buffering
  gSPSetGeometryMode(displayListPtr++, G_SHADE | G_SHADING_SMOOTH | G_ZBUFFER);

  // actually the triangles, using the specified vertices
  gSP2Triangles(displayListPtr++,0,1,2,0,0,2,3,0);

  // Mark that we've finished sending commands for this particular primitive.
  // This is needed to prevent race conditions inside the rendering hardware in 
  // the case that subsequent commands change rendering settings.
  gDPPipeSync(displayListPtr++);

  // pop the matrix that we added back off the stack, to move the drawing position 
  // back to where it was before we rendered this object
  gSPPopMatrix(displayListPtr++, G_MTX_MODELVIEW);
}

// the 'update' function
void update00() {
  // read controller input
  nuContDataGetEx(contdata, 0);
  if (contdata[0].trigger & A_BUTTON){
    squaresRotationDirection = !squaresRotationDirection;
  }

  // update square rotations
  {
    int i;
    for (i = 0; i < NUM_SQUARES; ++i)
    {
      squaresRotations[i] = (squaresRotations[i] + (squaresRotationDirection ? 1 : -1)) % 360;
    }
  }
}

// the nusystem callback for the stage, called once per frame
void stage00(int pendingGfx)
{
  // produce a new displaylist (unless we're running behind, meaning we already
  // have the maximum queued up)
  if(pendingGfx < 1)
    makeDL00();

  // update the state of the world for the next frame
  update00();
}
