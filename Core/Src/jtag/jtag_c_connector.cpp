/*
 * jtag_c_connector.cpp
 *
 *  Created on: May 27, 2021
 *      Author: anton.krug@gmail.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "jtag.hpp"

void jtag_tap_display() {
  jtag::tap::display();
}

#ifdef __cplusplus
}
#endif
