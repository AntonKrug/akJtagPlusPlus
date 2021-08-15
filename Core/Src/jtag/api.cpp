/*
 * api.cpp
 *
 *  Created on: Aug 15, 2021
 *      Author: anton.krug@gmail.com
 */


#include <array>

#include "api.hpp"
#include "bitbang.hpp"


namespace jtag {

  namespace api {

    tap::stateE defaultEndState = tap::stateE::RunTestIdle;

    // https://stackoverflow.com/questions/45650099
    uint8_t irOpcodeLen = 4;

    // https://techoverflow.net/2014/09/26/reading-stm32-unique-device-id-using-openocd/
    // https://www.element14.com/community/docs/DOC-60353/l/stmicroelectronics-bsdl-files-for-stm32-boundary-scan-description-language
    uint8_t drOpcodeLen = 32;


    requestAndResponse nop(uint32_t *req, uint32_t *res) {
      return JTAG_COMBINE_REQ_RES(req, res);
    }

    requestAndResponse failure(uint32_t *req, uint32_t *res) {
      // API failure handler is currently the same as the NOP implementation,
      // however it can be used with a debugger to separate the regular valid
      // stop command from a abnormal API call
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
      auto endState = (tap::stateE)(*req);
      req++;

      defaultEndState = endState;
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    requestAndResponse pathMove(uint32_t *req, uint32_t *res) {
      auto endState = (tap::stateE)(*req);
      req++;

      tap::stateMove(endState);
      return JTAG_COMBINE_REQ_RES(req, res);
    }


    requestAndResponse runTest(uint32_t *req, uint32_t *res) {
      uint32_t count = *req;
      req++;

      for (uint32_t i = 0; i < count; i++) {
        tap::stateMove(tap::stateE::RunTestIdle);
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


      enum class captureE:bool {
        ir = 0,
        dr = 1
      };


      enum class accessE:bool {
        write        = 0,
        readAndWrite = 1
      };


      enum class opcodeLengthE:bool {
        useGlobal      = 0,
        readFromStream = 1
      };


      enum class endstateE:bool {
        useGlobal      = 0,
        readFromStream = 1
      };


      enum class lenSizeFitsE:bool {
        fitsInto32  = 0,
        over32      = 1
      };


      template<captureE capture, accessE access, endstateE endstate, opcodeLengthE opcodeLength, lenSizeFitsE lenSize>
      requestAndResponse generic(uint32_t *req, uint32_t *res) {
        // Arguments in the stream are DATA, [LEN], [END_STATE]
        uint32_t data = *req;
        req++;

        if (capture == captureE::ir) {
          tap::stateMove(tap::stateE::ShiftIr);
        } else {
          tap::stateMove(tap::stateE::ShiftDr);
        }

        uint32_t length;
        if (opcodeLength == opcodeLengthE::useGlobal) {
          // Use the global length
          length = (capture == captureE::ir) ? irOpcodeLen : drOpcodeLen;
        } else {
          // Specified your own length in the packet
          length = *req;
          req++;
        }

        uint32_t read = bitbang::shiftTdi(length, data);

        if (access == accessE::readAndWrite) {
          // Send back to the USB what you read
          *res=read;
          res++;
        }

        if (endstate == endstateE::useGlobal) {
          // Go to the globally specified end state
          tap::stateMove(defaultEndState);
        } else {
          // Read from the request packet what end state should go to
          auto endState = (tap::stateE)(*req);
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
      commandE commandId = static_cast<commandE>(COMMAND_ID & 0b0000'1111);

      switch (commandId) {

        case commandE::nop: {
          ret = nop(req, res);
          break;
        }

        case commandE::ping: {
          ret = ping(req, res);
          break;
        }

        case commandE::reset: {
          ret = reset(req, res);
          break;
        }

        case commandE::stateMove: {
          ret = stateMove(req, res);
          break;
        }

        case commandE::pathMove: {
          ret = pathMove(req, res);
          break;
        }

        case commandE::runTest: {
          ret = runTest(req, res);
          break;
        }

        case commandE::setIrOpcodeLen: {
          ret = setIrOpcodeLen(req, res);
          break;
        }

        case commandE::setDrOpcodeLen: {
          ret = setDrOpcodeLen(req, res);
          break;
        }

        case commandE::scan: {
          // Take the higher 4-bits and calculate what SCAN variation it would be
          const uint32_t scanVariation = COMMAND_ID & 0b1111'0000;

          // Break up the SCAN variation into its components
          const auto isDr           = static_cast<scan::captureE>(     scanVariation & (1u << static_cast<uint8_t>(scanBitsE::isDr)));
          const auto isReadWrite    = static_cast<scan::accessE>(      scanVariation & (1u << static_cast<uint8_t>(scanBitsE::isReadWrite)));
          const auto isLenOpcode    = static_cast<scan::opcodeLengthE>(scanVariation & (1u << static_cast<uint8_t>(scanBitsE::isLenArgument)));
          const auto isLenFitInto32 = static_cast<scan::lenSizeFitsE>( scanVariation & (1u << static_cast<uint8_t>(scanBitsE::isLenOver32)));

          scan::generic<isDr, isReadWrite, scan::endstateE::useGlobal, isLenOpcode, isLenFitInto32>(req, res);
          break;
        }

        default: {
          // All invalid or unimplemented calls will cause failure
          ret = failure(req, res);
          break;
        }

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


    const std::array<commandHandler, 256> handlers = populateApiTable<0>();

  }
}

