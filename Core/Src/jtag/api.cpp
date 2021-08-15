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

    tap::state_e defaultEndState = tap::state_e::RunTestIdle;

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
        ir = 0,
        dr = 1
      };


      enum class access_e:bool {
        write        = 0,
        readAndWrite = 1
      };


      enum class opcodeLength_e:bool {
        useGlobal      = 0,
        readFromStream = 1
      };


      enum class endstate_e:bool {
        useGlobal      = 0,
        readFromStream = 1
      };


      enum class lenSizeFits_e:bool {
        fitsInto32  = 0,
        over32      = 1
      };


      template<capture_e capture, access_e access, endstate_e endstate, opcodeLength_e opcodeLength, lenSizeFits_e lenSize>
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
      command_e commandId = static_cast<command_e>(COMMAND_ID & 0b0000'1111);

      switch (commandId) {
        case command_e::nop: {
          ret = nop(req, res);
          break;
        }

        case command_e::ping: {
          ret = ping(req, res);
          break;
        }

        case command_e::reset: {
          ret = reset(req, res);
          break;
        }

        case command_e::stateMove: {
          ret = stateMove(req, res);
          break;
        }

        case command_e::pathMove: {
          ret = pathMove(req, res);
          break;
        }

        case command_e::runTest: {
          ret = runTest(req, res);
          break;
        }

        case command_e::setIrOpcodeLen: {
          ret = setIrOpcodeLen(req, res);
          break;
        }

        case command_e::setDrOpcodeLen: {
          ret = setDrOpcodeLen(req, res);
          break;
        }

        case command_e::scan: {
          // Take the higher 4-bits and calculate what SCAN variation it would be
          const uint32_t scanVariation = COMMAND_ID & 0b1111'0000;

          // Break up the SCAN variation into its components
          const auto isDr           = static_cast<scan::capture_e>(     scanVariation & (1u << static_cast<uint8_t>(scan_bits_e::isDr)));
          const auto isReadWrite    = static_cast<scan::access_e>(      scanVariation & (1u << static_cast<uint8_t>(scan_bits_e::isReadWrite)));
          const auto isLenOpcode    = static_cast<scan::opcodeLength_e>(scanVariation & (1u << static_cast<uint8_t>(scan_bits_e::isLenArgument)));
          const auto isLenFitInto32 = static_cast<scan::lenSizeFits_e>( scanVariation & (1u << static_cast<uint8_t>(scan_bits_e::isLenOver32)));

          scan::generic<isDr, isReadWrite, scan::endstate_e::useGlobal, isLenOpcode, isLenFitInto32>(req, res);
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

