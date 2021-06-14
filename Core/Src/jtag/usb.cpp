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

    // If the terminationValue is set to 0 and used in handleRquest, then
    // there is no need to check for buffer overflows. Because the command
    // handlers only increment the buffer as much as they process, the API
    // has dedicated command ID 0 to end processing and the handleQueue
    // will finish processing naturally without doing extra checks.
    uint32_t requestBuf[JTAG_USB_REPORT_SIZE];
    uint32_t terminationValue = 0;

    // No need to check for overflows and no need to have termination,
    // each single command consumes equal or more bytes from the request buffer
    // than it can produce into the response buffer.
    uint32_t responseBuf[JTAG_USB_REPORT_SIZE];

    // https://stackoverflow.com/questions/45650099
    uint8_t irOpcodeLen = 4;

    // https://techoverflow.net/2014/09/26/reading-stm32-unique-device-id-using-openocd/
    // https://www.element14.com/community/docs/DOC-60353/l/stmicroelectronics-bsdl-files-for-stm32-boundary-scan-description-language
    uint8_t drOpcodeLen = 32;


    uint32_t* skip(uint32_t *reqHandle, uint32_t *resHandle) {
      return reqHandle;
    }


    // Respond to ping with own FW version
    uint32_t* ping(uint32_t *reqHandle, uint32_t *resHandle) {
      *resHandle=JTAG_FW_VERSION;
      return reqHandle;
    }


    uint32_t* reset(uint32_t *reqHandle, uint32_t *resHandle) {
      uint32_t type = *reqHandle;
      reqHandle++;

      bitbang::resetSignal(type, -1);
      return reqHandle;
    }


    uint32_t* stateMove(uint32_t *reqHandle, uint32_t *resHandle) {
      auto endState = (tap::state_e)(*reqHandle);
      reqHandle++;

      defaultEndState = endState;
      return reqHandle;
    }


    uint32_t* pathMove(uint32_t *reqHandle, uint32_t *resHandle) {
      auto endState = (tap::state_e)(*reqHandle);
      reqHandle++;

      tap::stateMove(endState);
      return reqHandle;
    }


    uint32_t* runTest(uint32_t *reqHandle, uint32_t *resHandle) {
      uint32_t count = *reqHandle;
      reqHandle++;

      for (uint32_t i = 0; i < count; i++) {
        tap::stateMove(tap::state_e::RunTestIdle);
      }
      return reqHandle;
    }


    uint32_t* setIrOpcodeLen(uint32_t *reqHandle, uint32_t *resHandle) {
      irOpcodeLen = *reqHandle;
      reqHandle++;
      return reqHandle;
    }


    uint32_t* setDrOpcodeLen(uint32_t *reqHandle, uint32_t *resHandle) {
      drOpcodeLen = *reqHandle;
      reqHandle++;
      return reqHandle;
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
      uint32_t* generic(uint32_t *reqHandle, uint32_t *resHandle) {
        // Arguments in the stream are DATA, [LEN], [END_STATE]
        uint32_t data = *reqHandle;
        reqHandle++;

        if (capture == capture_e::ir) {
          tap::stateMove(tap::state_e::ShiftIr);
        } else {
          tap::stateMove(tap::state_e::ShiftDr);
        }

        uint32_t length;
        if (opcodeLength == opcodeLength_e::useGlobal) {
          // Use the global length
          length = (capture == capture_e::ir) ? irOpcodeLen : drOpcodeLen;
        } else {
          // Specified your own length in the packet
          length = *reqHandle;
          reqHandle++;
        }

        uint32_t read = bitbang::shiftTdi(length, data);

        if (access == access_e::readAndWrite) {
          // Send back to the USB what you read
          *resHandle=read;
          resHandle++;
        }

        if (endstate == endstate_e::useGlobal) {
          // Go to the globally specified end state
          tap::stateMove(defaultEndState);
        } else {
          // Read from the request packet what end state should go to
          auto endState = (tap::state_e)(*reqHandle);
          reqHandle++;
          tap::stateMove(endState);
        }
        return reqHandle;
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

    uint32_t response_sizes[api_e_size] = {
        0,      // end_processing

        1,
        0,
        0,      // setLed
        0,      // setTCK
        1,      // getTCK

        0,
        0,
        0,

        0,
        0,

        1,
        1,
        1,
        1,

        1,
        1,
        1,
        1,
    };



    void parseQueue(uint32_t *req, uint32_t *res) {
      // Handling of only non-zero buffers implemented
      // means that I can read the first command blindly
      // and do while checks later (for the next command in queue)
      // should save one check
      uint32_t commandId = *req;

      while (commandId == 0 && commandId < api_e_size)  {
        req++;
        // Invoke the command from the API function table
        req = handlers[commandId](req, res);
        res += response_sizes[commandId];

        commandId = *req;
      }

    }


  }
}




