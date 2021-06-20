/*
 * c_connector.cpp
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 *     License: GPLv2
 */


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "jtag_c_connector.h"
#include "bitbang.hpp"
#include "usb.hpp"
#include "stm32f429i_discovery_lcd.h"


#ifdef JTAG_TAP_TELEMETRY
void jtag_tap_telemetry_dispay() {
  jtag::tap::telemetry::displayStateMachineDiagram();
}
#endif

void jtag_setup() {

}


uint32_t requestBuf[JTAG_USB_REPORT_SIZE + 1] = { 0 };
uint32_t responseBuf[JTAG_USB_REPORT_SIZE]    = { 0 };


requestAndResponse jtag_usb_parseQueue(uint32_t *req, uint32_t *res) {
  return jtag::usb::parseQueue(req, res);
}


void jtag_loop() {

  jtag::bitbang::resetSignal(0, -1);

  jtag::tap::resetSM();  // Somewhat redundant, after target reset the tap would be in the reset anyway
  jtag::tap::stateMove(jtag::tap::state_e::ShiftDr);
  uint32_t IDcode = jtag::bitbang::shiftTdi(32, 0xdead'beef);

  uint32_t* req = requestBuf;
  uint32_t* res = responseBuf;

  jtag::usb::parseQueue(req, res);

  char buf[30];
  sprintf(buf, "ID = 0x%08X", (unsigned int)(IDcode));

  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_DisplayString(5, 270, buf);

  for (int number = 0; number < jtag::tap::state_e_size; number++) {
    jtag::tap::stateMove(static_cast<jtag::tap::state_e>(number));
    jtag::bitbang::shiftTms({8, 0b11111111});
    jtag::bitbang::shiftTmsRaw(32, number);
  }

  jtag::tap::stateMove(jtag::tap::state_e::RunTestIdle);
  jtag::tap::stateMove(jtag::tap::state_e::UpdateIr);

  jtag::tap::telemetry::statsDisplayCallsAndTime();
}


#ifdef __cplusplus
}
#endif
