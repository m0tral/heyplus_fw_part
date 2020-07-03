/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#if (GFX_USE_GINPUT && GINPUT_NEED_TOGGLE) /*|| defined(__DOXYGEN__)*/

#include "../../../../src/ginput/ginput_driver_toggle.h"

const GToggleConfig GInputToggleConfigTable[GINPUT_TOGGLE_CONFIG_ENTRIES];


void ginput_lld_toggle_init(const GToggleConfig *ptc) {
	// palSetGroupMode(((IOBus *)ptc->id)->portid, ptc->mask, 0, ptc->mode);
			//ptc->id = &GDISP_WIN32[ptc - GInputToggleConfigTable];
		((GToggleConfig *)ptc)->id = 0;

		// We have 8 buttons per window.
		((GToggleConfig *)ptc)->mask = 0xFF;

		// No inverse or special mode
		((GToggleConfig *)ptc)->invert = 0x00;
		((GToggleConfig *)ptc)->mode = 0;
}

unsigned ginput_lld_toggle_getbits(const GToggleConfig *ptc) {
	// return palReadBus((IOBus *)ptc->id);
	return 0;
}

#endif /* GFX_USE_GINPUT && GINPUT_NEED_TOGGLE */
