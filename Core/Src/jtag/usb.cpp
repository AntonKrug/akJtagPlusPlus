/*
 * usb.cpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */


#include <array>

#include "usb.hpp"
#include "bitbang.hpp"


namespace jtag {

  namespace usb {

    tap::state_e defaultEndState = tap::state_e::RunTestIdle;

    bool processBuffer = true;

    // If the terminationValue is set to 0 and used in handleRquest, then
    // there is no need to check for buffer overflows. Because the command
    // handlers only increment the buffer as much as they process, the API
    // has dedicated command ID 0 to end processing and the handleQueue
    // will finish processing naturally without doing extra checks.
    uint32_t requestBuf[JTAG_USB_REPORT_SIZE + 1] = { 0 };

    // No need to check for overflows and no need to have termination,
    // each single command consumes equal or more bytes from the request buffer
    // than it can produce into the response buffer.
    uint32_t responseBuf[JTAG_USB_REPORT_SIZE];

    // https://stackoverflow.com/questions/45650099
    uint8_t irOpcodeLen = 4;

    // https://techoverflow.net/2014/09/26/reading-stm32-unique-device-id-using-openocd/
    // https://www.element14.com/community/docs/DOC-60353/l/stmicroelectronics-bsdl-files-for-stm32-boundary-scan-description-language
    uint8_t drOpcodeLen = 32;


    requestAndResponse stop(uint32_t *req, uint32_t *res) {
      processBuffer = false;
      return JTAG_COMBINE_REQ_RES(req, res);
    }

    requestAndResponse failure(uint32_t *req, uint32_t *res) {
      // API failure handler is currently the same as the stop implementation,
      // however it can be used with a debugger to separate the regular valid
      // stop command from a abnormal API call
      processBuffer = false;
      return JTAG_COMBINE_REQ_RES(req, res);
    }

    requestAndResponse skip(uint32_t *req, uint32_t *res) {
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    // Respond to ping with own FW version
    requestAndResponse ping(uint32_t *req, uint32_t *res) {
      *res=JTAG_FW_VERSION;
      res++;
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    requestAndResponse reset(uint32_t *req, uint32_t *res) {
      uint32_t type = *req;
      req++;

      bitbang::resetSignal(type, -1);
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    requestAndResponse stateMove(uint32_t *req, uint32_t *res) {
      auto endState = (tap::state_e)(*req);
      req++;

      defaultEndState = endState;
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    requestAndResponse pathMove(uint32_t *req, uint32_t *res) {
      auto endState = (tap::state_e)(*req);
      req++;

      tap::stateMove(endState);
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    requestAndResponse runTest(uint32_t *req, uint32_t *res) {
      uint32_t count = *req;
      req++;

      for (uint32_t i = 0; i < count; i++) {
        tap::stateMove(tap::state_e::RunTestIdle);
      }
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    requestAndResponse setIrOpcodeLen(uint32_t *req, uint32_t *res) {
      irOpcodeLen = *req;
      req++;
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    requestAndResponse setDrOpcodeLen(uint32_t *req, uint32_t *res) {
      drOpcodeLen = *req;
      req++;
      return JTAG_COMBINE_REQ_RES(req, res);
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
      requestAndResponse generic(uint32_t *req, uint32_t *res) {
        // Arguments in the stream are DATA, [LEN], [END_STATE]
        uint32_t data = *req;
        req++;

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
          length = *req;
          req++;
        }

        uint32_t read = bitbang::shiftTdi(length, data);

        if (access == access_e::readAndWrite) {
          // Send back to the USB what you read
          *res=read;
          res++;
        }

        if (endstate == endstate_e::useGlobal) {
          // Go to the globally specified end state
          tap::stateMove(defaultEndState);
        } else {
          // Read from the request packet what end state should go to
          auto endState = (tap::state_e)(*req);
          req++;
          tap::stateMove(endState);
        }
        return JTAG_COMBINE_REQ_RES(req, res);
      }

    }


    template<uint8_t COMMAND_ID>
    constexpr requestAndResponse apiSwitch(uint32_t *req, uint32_t *res) {
      requestAndResponse ret;

      // Take only the lower 4-bits from the command and turn it into a ENUM
      api_e commandId = static_cast<api_e>(COMMAND_ID & 0b0000'1111);

      switch (commandId) {
        case api_e::end_processing:
          ret = stop(req, res);
          break;

        case api_e::ping:
          ret = ping(req, res);
          break;

        case api_e::reset:
          ret = reset(req, res);
          break;

        case api_e::stateMove:
          ret = stateMove(req, res);
          break;

        case api_e::pathMove:
          ret = pathMove(req, res);
          break;

        case api_e::runTest:
          ret = runTest(req, res);
          break;

        case api_e::setIrOpcodeLen:
          ret = setIrOpcodeLen(req, res);
          break;

        case api_e::setDrOpcodeLen:
          ret = setDrOpcodeLen(req, res);
          break;

        case api_e::scan:
          // TODO: implement
          break;

        default:
          // All invalid or unimplemented calls will cause failure
          ret = failure(req, res);
          break;
      }
      return ret;
    }


    template <uint32_t lookupIndex>
    constexpr std::array<commandHandler, 256> populateApiTable() {
        auto result = populateApiTable<lookupIndex + 1>();
        result[lookupIndex] = apiSwitch<lookupIndex>;

        return result;
    }


    template <>
    constexpr std::array<commandHandler, 256> populateApiTable<256>() {
        std::array<commandHandler, 256> lookupTable = { &failure };
        return lookupTable;
    }


    const auto handlers = populateApiTable<0>();

    requestAndResponse parseQueue(uint32_t *req, uint32_t *res) {
      // Handling only non-zero buffers means that I can read the first command blindly
      uint32_t commandIds = *req;

      processBuffer = true;

      while (processBuffer) {
        uint8_t commandId = commandIds & 0xff;  // take only the lowest 8-bit from the IDs
        commandIds = commandIds >> 8;           // move the IDs so next time the next 8-bits can be loaded

        // Advance the pointer in the request stream, so the invoked functions
        // will already have request stream pointing to their arguments (and not their commandId)
        req++;

        // Invoke the command from the API function table
        requestAndResponse combined = handlers[commandId](req, res);

        // Take the combined returned value and assign it back to the request and response pointers
        JTAG_DECOMPOSE_REQ_RES(combined, req, res);
      }

      // Return back the pointers, subtracting them later from the originals will tell us
      // how much of the packet was processed and how much response was populated
      return JTAG_COMBINE_REQ_RES(req, res);
    }


  }
}




