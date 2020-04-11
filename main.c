
#include <nusys.h>
#include "stage00.h"


NUContData  contdata[1]; /* Read data of 1 controller */

void mainproc(void)
{
  // initialize the graphics system
  nuGfxInit();

  // initialize the controller manager
  nuContInit();

  // set the callback to be called every frame
  nuGfxFuncSet((NUGfxFunc)stage00);

  nuGfxDisplayOn();

  // send this thread into an infinite loop
  while(1)
    ;
}

