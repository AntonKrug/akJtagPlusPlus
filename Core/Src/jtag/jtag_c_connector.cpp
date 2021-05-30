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

#include "jtag_c_connector.h"
#include "bitbang.hpp"


#ifdef JTAG_TAP_TELEMETRY
void jtag_tap_telemetry_dispay() {
  jtag::tap::telemetry::displayStateMachineDiagram();
}
#endif

void jtag_setup() {

}


void jtag_loop() {

  jtag::bitbang::resetTarget(32);

  jtag::tap::reset();  // Somewhat redundant, after target reset the tap would be in the reset anyway
  jtag::tap::stateMove(jtag::tap::state_e::ShiftIr);
  auto IDcode = jtag::bitbang::shiftTdi(32, 0x0000'0000);

  for (int number = 0; number < jtag::tap::tapStateSize; number++) {
    jtag::tap::stateMove(static_cast<jtag::tap::state_e>(number));
    jtag::bitbang::shiftTms({8, 0b11111111});
    jtag::bitbang::shiftTmsRaw(32, number);
  }

  jtag::tap::stateMove(jtag::tap::state_e::RunTestIdle);
  jtag::tap::stateMove(jtag::tap::state_e::UpdateIr);
}


#ifdef __cplusplus
}
#endif
