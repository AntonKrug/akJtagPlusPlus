/*
 * jtag_c_connector.cpp
 *
 *  Created on: May 27, 2021
 *      Author: anton.krug@gmail.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "tap.hpp"

void jtag_tap_display() {
  jtag::tap::telemetry::display();
}

#ifdef __cplusplus
}
#endif
