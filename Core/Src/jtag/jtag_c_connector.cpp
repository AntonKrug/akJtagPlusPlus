/*
 * c_connector.cpp
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 */


#ifdef __cplusplus
extern "C" {
#endif

#include "jtag_c_connector.h"
#include "bitbang.hpp"


void jtag_setup() {

}


void jtag_loop() {
  jtag::bitbang::demo();
}


#ifdef __cplusplus
}
#endif
