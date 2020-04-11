

#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

// The screen size  
#define SCREEN_HT        240
#define SCREEN_WD        320

#define MAX_GRAPHICS_TASKS 2
// The maximum length of the display list of one task  
#define MAX_DISPLAY_LIST_COMMANDS 2048
#define MAX_OBJECTS 10

#define fovy 45
#define aspect (f32)SCREEN_WD/(f32)SCREEN_HT
#define nearPlane 10
#define farPlane 1000


// a 3d position, such as that of an object
typedef struct Vec3d {
  float x;
  float y;
  float z;
} Vec3d;


// a struct to hold graphics data used by the RCP which can change at runtime
typedef struct GraphicsState {
  Mtx projection;
  Mtx modelview;
  Mtx objectTransforms[MAX_OBJECTS];
} GraphicsState;

extern int graphicsTaskNum;
extern struct GraphicsState graphicsStates[MAX_GRAPHICS_TASKS];
extern struct GraphicsState * curGraphicsState;
extern Gfx displayLists[MAX_GRAPHICS_TASKS][MAX_DISPLAY_LIST_COMMANDS];
// The tail of the displaylist we are currently working on
// We use a global variable because otherwise you'd need to pass this around
// (by reference) a lot. Welcome to 90s-style programming!
extern Gfx * displayListPtr;

extern void gfxRCPInit(void);
extern void gfxClearCfb(void);

extern Gfx setup_rdpstate[];
extern Gfx setup_rspstate[];

#endif /* _GRAPHIC_H_ */



