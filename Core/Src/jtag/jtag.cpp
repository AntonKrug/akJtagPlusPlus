/*
 * jtag.cpp
 *
 *  Created on: May 27, 2021
 *      Author: anton.krug@gmail.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f429i_discovery_lcd.h"


namespace jtag {

	namespace tap {

		// https://image.slidesharecdn.com/jtagpresentation-100723072934-phpapp01/95/jtag-presentation-17-728.jpg?cb=1279870813
		enum class tapState_e:uint32_t {
			TestLogicReset,
			RunTestIdle,
			SelectDrScan,
			CaptureDr,
			ShiftDr,
			Exit1Dr,
			PauseDr,
			Exit2Dr,
			UpdateDr,
			SelectIrScan,
			CaptureIr,
			ShiftIr,
			Exit1Ir,
			PauseIr,
			Exit2Ir,
			UpdateIr
		};

		struct display_entry_s {
			const char*       name;
			const uint32_t    color;
			const uint16_t    x;
			const uint16_t    y;
		};

		const uint16_t rowFirstX   = 5;
		const uint16_t blockWidth  = 100;
		const uint16_t rowSecondX  = 120;
		const uint16_t lineSpacing = 10;
		const uint16_t fontHeight  = 8;
		const uint16_t lineHeight  = fontHeight + lineSpacing;

		const display_entry_s displayEntries[] = {
				{ "Test Logic Reset", LCD_COLOR_BROWN,      rowFirstX,  0 * lineHeight + lineSpacing },
				{ "Run Test / Idle",  LCD_COLOR_BROWN,      rowFirstX,  1 * lineHeight + lineSpacing  },

				{ "Select DR Scan",   LCD_COLOR_DARKBLUE,   rowFirstX,  3 * lineHeight },
        { "Shift DR",         LCD_COLOR_DARKBLUE,   rowFirstX,  4 * lineHeight },
        { "Exit 1 DR",        LCD_COLOR_DARKBLUE,   rowFirstX,  5 * lineHeight },
        { "Pause DR",         LCD_COLOR_DARKBLUE,   rowFirstX,  6 * lineHeight },
        { "Exit 2 DR",        LCD_COLOR_DARKBLUE,   rowFirstX,  7 * lineHeight },
        { "Update DR",        LCD_COLOR_DARKBLUE,   rowFirstX,  8 * lineHeight },

        { "Select IR Scan",   LCD_COLOR_DARKGREEN,  rowSecondX, 3 * lineHeight },
        { "Shift IR",         LCD_COLOR_DARKGREEN,  rowSecondX, 4 * lineHeight },
        { "Exit 1 IR",        LCD_COLOR_DARKGREEN,  rowSecondX, 5 * lineHeight },
        { "Pause IR",         LCD_COLOR_DARKGREEN,  rowSecondX, 6 * lineHeight },
        { "Exit 2 IR",        LCD_COLOR_DARKGREEN,  rowSecondX, 7 * lineHeight },
        { "Update IR",        LCD_COLOR_DARKGREEN,  rowSecondX, 8 * lineHeight },
		};

		void display() {
		  for (auto entry: displayEntries) {
		    BSP_LCD_SetBackColor(entry.color);

        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        BSP_LCD_FillRect(entry.x -2, entry.y - 2, blockWidth + 4, fontHeight + 3);

        BSP_LCD_SetTextColor(entry.color);
        BSP_LCD_FillRect(entry.x -1, entry.y - 1, blockWidth + 2, fontHeight + 1);

        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		    BSP_LCD_DisplayString(entry.x, entry.y, entry.name);
		  }
		}


	}
}

#ifdef __cplusplus
}
#endif
