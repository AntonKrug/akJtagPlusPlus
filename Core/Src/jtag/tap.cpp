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
#include "tap.hpp"
#include "bitbang.hpp"


namespace jtag {

	namespace tap {

	  tapState_e currentState = tapState_e::TestLogicReset;

		tmsMove tapMoves [tapStateSize][tapStateSize] = {

		    // Lookup table with how many TCK clocks and what TMS bits has to be shifted to move from one
		    // state to the other state inside the TAP state machine
		    //
		    // https://www.allaboutcircuits.com/uploads/articles/jtag-part-ii-the-test-access-port-state-machine-SG-aac-image1.jpg
		    //
		    //      End State => TLReset       RunTestIdle SelectDR     CaptureDR    ShiftDR       Exit1DR       PauseDR        Exit2DR         UpdateDR       SelectIR     CaptureIR     ShiftIR        Exit1IR        PauseIR         Exit2IR          UpdateIR
		    //
		    // Start state
		    //  |
		    //  V
		    /* TLReset     */ {  {1, 0b1},     {1, 0b0},   {2, 0b10},   {3, 0b010},  {4, 0b0010},  {4, 0b1010},  {5, 0b01010},  {6, 0b101010},  {5, 0b11010},  {3, 0b110},  {4, 0b0110},  {5, 0b00110},  {5, 0b10110},  {6, 0b010110},  {7, 0b1010110},  {6, 0b110110}   },
		    /* RunTestIdle */ {  {3, 0b111},   {1, 0b0},   {1, 0b1},    {2, 0b01},   {3, 0b001},   {3, 0b101},   {4, 0b0101},   {5, 0b10101},   {4, 0b1101},   {2, 0b11},   {3, 0b011},   {4, 0b0011},   {4, 0b1011},   {5, 0b01011},   {6, 0b101011},   {5, 0b11011}    },
		    /* SelectDR    */ {  {2, 0b11},    {3, 0b011}, {4, 0b1011}, {1, 0b0},    {2, 0b00},    {2, 0b10},    {3, 0b010},    {4, 0b1010},    {3, 0b110},    {1, 0b1},    {2, 0b01},    {3, 0b001},    {3, 0b101},    {4, 0b0101},    {5, 0b10101},    {4, 0b1101}     },
		    /* CaptureDR   */ {  {5, 0b11111}, {3, 0b011}, {3, 0b111},  {4, 0b0111}, {1, 0b0},     {1, 0b1},     {2, 0b01},     {3, 0b101},     {2, 0b11},     {4, 0b1111}, {5, 0b01111}, {6, 0b001111}, {6, 0b101111}, {7, 0b0101111}, {8, 0b10101111}, {7, 0b1101111}  },
		    /* ShiftDR     */ {  {5, 0b11111}, {3, 0b011}, {3, 0b111},  {4, 0b0111}, {1, 0b0},     {1, 0b1},     {2, 0b01},     {3, 0b101},     {2, 0b11},     {4, 0b1111}, {5, 0b01111}, {6, 0b001111}, {6, 0b101111}, {7, 0b0101111}, {8, 0b10101111}, {7, 0b1101111}  },
		    /* Exit1DR     */ {  {4, 0b1111},  {2, 0b01},  {2, 0b11},   {3, 0b011},  {3, 0b010},   {4, 0b1010},  {1, 0b0},      {2, 0b10},      {1, 0b1},      {3, 0b111},  {4, 0b0111},  {5, 0b00111},  {5, 0b10111},  {6, 0b010111},  {7, 0b1010111},  {6, 0b110111}   },
		    /* PauseDR     */ {  {5, 0b11111}, {3, 0b011}, {3, 0b111},  {4, 0b0111}, {2, 0b01},    {3, 0b101},   {1, 0b0},      {1, 0b1},       {2, 0b11},     {4, 0b1111}, {5, 0b01111}, {6, 0b001111}, {6, 0b101111}, {7, 0b0101111}, {8, 0b10101111}, {7, 0b1101111}  },
		    /* Exit2DR     */ {  {4, 0b1111},  {2, 0b01},  {2, 0b11},   {3, 0b011},  {1, 0b0},     {2, 0b10},    {3, 0b010},    {4, 0b1010},    {1, 0b1},      {3, 0b111},  {4, 0b0111},  {5, 0b00111},  {5, 0b10111},  {6, 0b010111},  {7, 0b1010111},  {6, 0b110111}   },
		    /* UpdateDR    */ {  {3, 0b111},   {1, 0b0},   {1, 0b1},    {2, 0b01},   {3, 0b001},   {3, 0b101},   {4, 0b0101},   {5, 0b10101},   {4, 0b1101},   {2, 0b11},   {3, 0b011},   {4, 0b0011},   {4, 0b1011},   {5, 0b01011},   {6, 0b101011},   {5, 0b11011}    },
		    /* SelectIR    */ {  {1, 0b1},     {2, 0b01},  {3, 0b101},  {4, 0b0101}, {5, 0b00101}, {5, 0b10101}, {6, 0b010101}, {7, 0b1010101}, {6, 0b110101}, {4, 0b1101}, {1, 0b0},     {2, 0b00},     {2, 0b10},     {3, 0b010},     {4, 0b1010},     {3, 0b110}      },
		    /* CaptureIR   */ {  {5, 0b11111}, {3, 0b011}, {3, 0b111},  {4, 0b0111}, {5, 0b00111}, {5, 0b10111}, {6, 0b010111}, {7, 0b1010111}, {6, 0b110111}, {4, 0b1111}, {5, 0b01111}, {1, 0b0},      {1, 0b1},      {2, 0b01},      {3, 0b101},      {2, 0b11}       },
		    /* ShiftIR     */ {  {5, 0b11111}, {3, 0b011}, {3, 0b111},  {4, 0b0111}, {5, 0b00111}, {5, 0b10111}, {6, 0b010111}, {7, 0b1010111}, {6, 0b110111}, {4, 0b1111}, {5, 0b01111}, {1, 0b0},      {1, 0b1},      {2, 0b01},      {3, 0b101},      {2, 0b11}       },
		    /* Exit1IR     */ {  {4, 0b1111},  {2, 0b01},  {2, 0b11},   {3, 0b011},  {4, 0b0011},  {4, 0b1011},  {5, 0b01011},  {6, 0b101011},  {5, 0b11011},  {3, 0b111},  {4, 0b0111},  {3, 0b010},    {4, 0b1010},   {1, 0b0},       {2, 0b10},       {1, 0b1}        },
		    /* PauseIR     */ {  {5, 0b11111}, {3, 0b011}, {3, 0b111},  {4, 0b0111}, {5, 0b00111}, {5, 0b10111}, {6, 0b010111}, {7, 0b1010111}, {6, 0b110111}, {4, 0b1111}, {5, 0b01111}, {2, 0b01},     {3, 0b101},    {1, 0b0},       {1, 0b1},        {2, 0b11}       },
		    /* Exit2IR     */ {  {4, 0b1111},  {2, 0b01},  {2, 0b11},   {3, 0b011},  {4, 0b0011},  {4, 0b1011},  {5, 0b01011},  {6, 0b101011},  {5, 0b11011},  {3, 0b111},  {4, 0b0111},  {1, 0b0},      {2, 0b10},     {3, 0b010},     {4, 0b1010},     {1, 0b1}        },
		    /* UpdateIR    */ {  {3, 0b111},   {1, 0b0},   {1, 0b1},    {2, 0b01},   {3, 0b001},   {3, 0b101},   {4, 0b0101},   {5, 0b10101},   {4, 0b1101},   {2, 0b11},   {3, 0b011},   {4, 0b0011},   {4, 0b1011},   {5, 0b01011},   {6, 0b101011},   {5, 0b11011}    }
		};

		void reset() {
		  jtag::bitbang::shiftTms({8, 0b11111111});

		  currentState = tapState_e::TestLogicReset;
		}


		void stateMove(tapState_e whereToMove) {
		  auto whatToShift = tapMoves[static_cast<int>(currentState)][static_cast<int>(whereToMove)];
		  jtag::bitbang::shiftTms(whatToShift);
		  currentState = whereToMove;
		}


		namespace telemetry {

		  struct display_entry_s {
        const char*       name;
        const uint32_t    color;
        const uint16_t    x;
        const uint16_t    y;
      };


      const uint16_t rowFirstX   = 5;
      const uint16_t blockWidth  = 100;
      const uint16_t rowSecondX  = 120;
      const uint16_t lineSpacing = 14;
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
}

#ifdef __cplusplus
}
#endif
