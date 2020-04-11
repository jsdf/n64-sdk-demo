
#include <nusys.h>
#include "graphic.h"


int graphicsTaskNum = 0;
GraphicsTask graphicsTasks[MAX_GRAPHICS_TASKS];
Gfx displayLists[MAX_GRAPHICS_TASKS][MAX_DISPLAY_LIST_COMMANDS];
Gfx * displayListPtr;

GraphicsTask * gfxSwitchTask() {
  // switch the current graphics task
  graphicsTaskNum = (graphicsTaskNum + 1) % MAX_GRAPHICS_TASKS;
  displayListPtr = &displayLists[graphicsTaskNum][0];
  return &graphicsTasks[graphicsTaskNum];
}

// The initialization of RSP/RDP
void gfxRCPInit(void)
{
  // set the RSP segment register
  gSPSegment(displayListPtr++, 0, 0x0);  // For the CPU virtual address

  // init the RSP
  gSPDisplayList(displayListPtr++, OS_K0_TO_PHYSICAL(setup_rspstate));

  // init the RDP
  gSPDisplayList(displayListPtr++, OS_K0_TO_PHYSICAL(setup_rdpstate));
}


// clears the frame buffer and z buffer
void gfxClearCfb(void)
{
  // Clear the Z-buffer  
  gDPSetDepthImage(displayListPtr++, OS_K0_TO_PHYSICAL(nuGfxZBuffer));
  gDPSetCycleType(displayListPtr++, G_CYC_FILL);
  gDPSetColorImage(displayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b,SCREEN_WD,
		   OS_K0_TO_PHYSICAL(nuGfxZBuffer));
  gDPSetFillColor(displayListPtr++,(GPACK_ZDZ(G_MAXFBZ,0) << 16 |
			       GPACK_ZDZ(G_MAXFBZ,0)));
  gDPFillRectangle(displayListPtr++, 0, 0, SCREEN_WD-1, SCREEN_HT-1);
  gDPPipeSync(displayListPtr++);
  
  // Clear the frame buffer  
  gDPSetColorImage(displayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WD,
		   osVirtualToPhysical(nuGfxCfb_ptr));
  gDPSetFillColor(displayListPtr++, (GPACK_RGBA5551(0, 0, 0, 1) << 16 | 
				GPACK_RGBA5551(0, 0, 0, 1)));
  gDPFillRectangle(displayListPtr++, 0, 0, SCREEN_WD-1, SCREEN_HT-1);
  gDPPipeSync(displayListPtr++);
}
