#include "gfx.h"
#include "gui.h"


#if (WIN32_SIM_MK)
int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	gfxInit(); // call uGFX init
	gui_thread_entry();
	while(1){
		gfxSleepMilliseconds(1000);
	}
	return 0;
}
#endif