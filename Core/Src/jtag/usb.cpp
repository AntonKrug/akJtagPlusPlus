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

    // https://stackoverflow.com/questions/45650099
    uint8_t irOpcodeLen = 4;

    // https://techoverflow.net/2014/09/26/reading-stm32-unique-device-id-using-openocd/
    // https://www.element14.com/community/docs/DOC-60353/l/stmicroelectronics-bsdl-files-for-stm32-boundary-scan-description-language
    uint8_t drOpcodeLen = 32;


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
      auto endState = (tap::state_e)(**reqHandle);
      (*reqHandle)++;

      defaultEndState = endState;
    }


    void pathMove(uint32_t **reqHandle, uint32_t **resHandle) {
      auto endState = (tap::state_e)(**reqHandle);
      (*reqHandle)++;

      tap::stateMove(endState);
    }


    void runTest(uint32_t **reqHandle, uint32_t **resHandle) {
      uint32_t count = **reqHandle;
      (*reqHandle)++;

      for (uint32_t i = 0; i < count; i++) {
        tap::stateMove(tap::state_e::RunTestIdle);
      }
    }


    void setIrOpcodeLen(uint32_t **reqHandle, uint32_t **resHandle) {
      irOpcodeLen = **reqHandle;
      (*reqHandle)++;
    }


    void setDrOpcodeLen(uint32_t **reqHandle, uint32_t **resHandle) {
      drOpcodeLen = **reqHandle;
      (*reqHandle)++;
    }


    namespace scan {

      enum class capture_e:bool {
        ir,
        dr
      };

      enum class access_e:bool {
        write,
        readAndWrite
      };

      enum class opcodeLength_e:bool {
        useGlobal,
        readFromStream
      };

      enum class endstate_e:bool {
        useGlobal,
        readFromStream
      };

      template<capture_e capture, access_e access, endstate_e endstate, opcodeLength_e opcodeLength>
      void generic(uint32_t **reqHandle, uint32_t **resHandle) {
        // Arguments in the stream are DATA, [LEN], [END_STATE]

        uint32_t data = **reqHandle;
        (*reqHandle)++;

        if (capture == capture_e::ir) {
          tap::stateMove(tap::state_e::CaptureIr);
        } else {
          tap::stateMove(tap::state_e::CaptureDr);
        }

        uint8_t length;
        if (opcodeLength == opcodeLength_e::useGlobal) {
          length = (capture == capture_e::ir) ? irOpcodeLen : drOpcodeLen;
        } else {
          length = **reqHandle;
          (*reqHandle)++;
        }

        uint32_t read = bitbang::shiftTdi(length, data);

        if (access == access_e::readAndWrite) {
          **resHandle=read;
          (*resHandle)++;
        }

        if (endstate == endstate_e::useGlobal) {
          tap::stateMove(defaultEndState);
        } else {
          auto endState = (tap::state_e)(**reqHandle);
          tap::stateMove(endState);
        }

      }

    }


    commandHandler handlers[api_e_size] = {
        &skip,      // end_processing

        &ping,
        &reset,
        &skip,      // setLed
        &skip,      // setTCK
        &skip,      // getTCK

        &stateMove,
        &pathMove,
        &runTest,

        &setIrOpcodeLen,
        &setDrOpcodeLen,

        &scan::generic<scan::capture_e::ir, scan::access_e::readAndWrite, scan::endstate_e::useGlobal,      scan::opcodeLength_e::useGlobal>,
        &scan::generic<scan::capture_e::ir, scan::access_e::write,        scan::endstate_e::useGlobal,      scan::opcodeLength_e::useGlobal>,
        &scan::generic<scan::capture_e::dr, scan::access_e::readAndWrite, scan::endstate_e::useGlobal,      scan::opcodeLength_e::useGlobal>,
        &scan::generic<scan::capture_e::dr, scan::access_e::write,        scan::endstate_e::useGlobal,      scan::opcodeLength_e::useGlobal>,

        &scan::generic<scan::capture_e::ir, scan::access_e::readAndWrite, scan::endstate_e::readFromStream, scan::opcodeLength_e::useGlobal>,
        &scan::generic<scan::capture_e::ir, scan::access_e::write,        scan::endstate_e::readFromStream, scan::opcodeLength_e::useGlobal>,
        &scan::generic<scan::capture_e::dr, scan::access_e::readAndWrite, scan::endstate_e::readFromStream, scan::opcodeLength_e::useGlobal>,
        &scan::generic<scan::capture_e::dr, scan::access_e::write,        scan::endstate_e::readFromStream, scan::opcodeLength_e::useGlobal>,
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




