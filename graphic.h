

#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

// The screen size  
#define SCREEN_HT        240
#define SCREEN_WD        320

#define MAX_GRAPHICS_TASKS 2
// The maximum length of the display list of one task  
#define MAX_DISPLAY_LIST_COMMANDS 2048
// determines the number of matrices we allocate, because we need one for every
// object we want to position in the world, for each graphics task
#define MAX_OBJECTS 10

#define FOVY 45
#define ASPECT (f32)SCREEN_WD/(f32)SCREEN_HT
#define NEAR_PLANE 10
#define FAR_PLANE 1000

// a 3d position, such as that of an object
typedef struct Vec3d {
  float x;
  float y;
  float z;
} Vec3d;


// a struct to hold graphics data used by the RCP which can change at runtime
typedef struct GraphicsTask {
  Mtx projection;
  Mtx modelview;
  Mtx objectTransforms[MAX_OBJECTS];
  Gfx displayList[MAX_DISPLAY_LIST_COMMANDS];
} GraphicsTask;

extern struct GraphicsTask graphicsTasks[MAX_GRAPHICS_TASKS];
extern struct GraphicsTask * curGraphicsTask;
extern Gfx * displayListPtr;

extern GraphicsTask * gfxSwitchTask();
extern void gfxRCPInit();
extern void gfxClearCfb();

extern Gfx setup_rdpstate[];
extern Gfx setup_rspstate[];

#endif /* _GRAPHIC_H_ */



