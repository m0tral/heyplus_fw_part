/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ryos.h"
#include "ry_utils.h"
#include "ry_type.h"
#include "ry_mcu_config.h"

/* Application Specific */
#include "gfx.h"
#include "gui.h"


systemticks_t gfxMillisecondsToTicks(delaytime_t ms)
{
	return ms;
}

systemticks_t gfxSystemTicks(void)
{
	return ryos_get_tick_count();
}



