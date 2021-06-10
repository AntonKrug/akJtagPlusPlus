/*
 * usb.cpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */


#include "usb.hpp"

namespace jtag {

  namespace usb {

    requestBuffer_s  request  = {};
    responseBuffer_s response = {};


    void skip(uint32_t **reqHandle, uint32_t **resHandle) {
      (*reqHandle)++;
    }


    // Respond to ping with own FW version
    void ping(uint32_t **reqHandle, uint32_t **resHandle) {
      (*reqHandle)++; // Just read the command from queue, point to the next command

      **resHandle=JTAG_FW_VERSION;
      (*resHandle)++;
    }


    commandHandler_s handlers[api_e_size] = {
        { &skip },
        { &ping },
    };


    void handleQueue(uint32_t *req, uint32_t *res) {

      uint32_t commandId = *req;
      req++;

      do {
        if (commandId >= api_e_size) break; // Command outside the API

        handlers[commandId].fun_ptr(&req, &res);

        commandId = *req;
        req++;
      } while (commandId);

    }


  }
}




