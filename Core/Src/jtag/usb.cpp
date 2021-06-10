/*
 * usb.cpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */


#include "usb.hpp"

namespace jtag {

  namespace usb {

    requestBuffer_s  request = {};
    responseBuffer_s response = {};

    uint8_t skip(uint32_t **reqHandle, uint32_t **resHandle) {
      *reqHandle++;
      return 1;
    }


    // Respond with own version to a ping
    uint8_t ping(uint32_t **reqHandle, uint32_t **resHandle) {
      *reqHandle++; // Just read the command from queue, point to the next command

      **resHandle=JTAG_FW_VERSION;
      *resHandle++;
      return 1;
    }

    commandHandler_s handlers[api_e_size] = {
        &skip,
        &ping,
    };

    void handleQueue(uint32_t **reqHandle, uint32_t **resHandle) {

      while (**reqHandle) {

      }
    }

  }
}




