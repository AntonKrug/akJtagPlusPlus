/*
 * usb.cpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */

#include <cstdint>
#include "usb.hpp"

namespace jtag {

  namespace usb {

    commandHandler_s handlers[api_e_size];

    void handleBuffer(uint32_t *buf) {
      while (*buf) {

      }
    }

  }
}




