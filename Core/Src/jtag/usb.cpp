/*
 * usb.cpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */


#include <array>

#include "usb.hpp"
#include "api.hpp"


namespace jtag {

  namespace usb {

    bool processBuffer = true;

    requestAndResponse parseQueue(uint32_t *req, uint32_t *res) {
      // Handling only non-zero buffers means that I can read the first command blindly
      uint32_t commandIds = *req;

      // Repeat while still we have some IDs in the combined ID
      while (commandIds) {
        uint8_t commandId = commandIds & 0xff;  // take only the lowest 8-bit from the IDs
        commandIds = commandIds >> 8;           // move the IDs so next time the next 8-bits can be loaded

        // Advance the pointer in the request stream, so the invoked functions
        // will already have request stream pointing to their arguments (and not their commandId)
        req++;

        // Invoke the command from the API function table
        requestAndResponse combined = jtag::api::handlers[commandId](req, res);

        // Take the combined returned value and assign it back to the request and response pointers
        JTAG_DECOMPOSE_REQ_RES(combined, req, res);
      }

      // Return back the pointers, subtracting them later from the originals will tell us
      // how much of the packet was processed and how much response was populated
      return JTAG_COMBINE_REQ_RES(req, res);
    }


  }
}




