/*
 * usb.cpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */


#include "usb.hpp"
#include "bitbang.hpp"

namespace jtag {

  namespace usb {

    tap::state_e defaultEndState = tap::state_e::RunTestIdle;

    requestBuffer_s  request  = {};
    responseBuffer_s response = {};


    void skip(uint32_t **reqHandle, uint32_t **resHandle) {
    }


    // Respond to ping with own FW version
    void ping(uint32_t **reqHandle, uint32_t **resHandle) {
      **resHandle=JTAG_FW_VERSION;  // Populate response
      (*resHandle)++;               // Increment response buffer
    }


    void reset(uint32_t **reqHandle, uint32_t **resHandle) {
      uint32_t type = **reqHandle;
      (*reqHandle)++;

      bitbang::resetSignal(type, -1);
    }


    void stateMove(uint32_t **reqHandle, uint32_t **resHandle) {
      uint32_t endState = **reqHandle;
      (*reqHandle)++;

      defaultEndState = (tap::state_e)endState;
    }


    commandHandler handlers[api_e_size] = {
        &skip,      // end_processing

        &ping,      // ping
        &reset,     // reset
        &skip,      // setLed
        &skip,      // setTCK
        &skip,      // getTCK

        &stateMove

    };


    void parseQueue(uint32_t *req, uint32_t *res) {
      // Handling of only non-zero buffers implemented
      // means that I can read the first command blindly
      // and do while checks later (for the next command in queue)
      // should save one check
      uint32_t commandId = *req;
      req++;

      do {
        if (commandId >= api_e_size) break; // Command outside the API

        // Invoke the command from the API function table
        handlers[commandId](&req, &res);

        commandId = *req;
        req++;
      } while (commandId);

    }


  }
}




