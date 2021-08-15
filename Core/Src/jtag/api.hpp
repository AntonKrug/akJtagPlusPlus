/*
 * api.hpp
 *
 *  Created on: Aug 15, 2021
 *      Author: Fredy
 */

#ifndef SRC_JTAG_API_HPP_
#define SRC_JTAG_API_HPP_

#include <cstdint>

#include "jtag_global.h"
#include "combined_request_response.h"

#ifdef __cplusplus
extern "C" {
#endif


namespace jtag {

  namespace api {

    extern const std::array<commandHandler, 256> handlers;


    enum class command_e:uint32_t {
      nop,            // do not do anything, process another call, or eventually stop

      ping,           // respond back what version this FW is
      reset,          // trst or srst
      setLed,
      setTCK,         // set TCK speed
      getTCK,         // get TCK speed

      stateMove,      // Specify endState after commands (don't change TAP SM)
      pathMove,       // move current state to the endstate (change TAP SM)
      runTest,

      setIrOpcodeLen, // max 32bits
      setDrOpcodeLen, // max 32bits
      scan,           // do all variations of scans, dedicated with 4-bits of API calls

      // Permutations of the 4-bits:

      // 4bit - IR/DR scan
      // 5bit - Write/Read+Write
      // 6bit - OpCodeLen Global / Argument
      // 7bit - OpCodeLen under or equal to 32-bit / OpCodeLen above 32-bit (but under or equal to 64-bit)

      // Read and write IR, 1 argument  uint32_t (uint32_t data) => (len and endState are global),      len <= 32
      // Write IR,          1 argument  void     (uint32_t data) => (len and endState are global),      len <= 32
      // Read and write DR, 1 argument  uint32_t (uint32_t data) => (len and endState are global),      len <= 32
      // Write DR,          1 argument  void     (uint32_t data) => (len and endState are global),      len <= 32
      //
      // Read and write IR, 2 arguments uint32_t (uint32_t len, uint32_t data) => (endState is global), len <= 32
      // Write IR,          2 arguments void     (uint32_t len, uint32_t data) => (endState is global), len <= 32
      // Read and write DR, 2 arguments uint32_t (uint32_t len, uint32_t data) => (endState is global), len <= 32
      // Write DR,          2 arguments void     (uint32_t len, uint32_t data) => (endState is global), len <= 32

      // Read and write IR, 1 argument  uint64_t (uint64_t data) => (len and endState are global),      64 >= len > 32
      // Write IR,          1 argument  void     (uint64_t data) => (len and endState are global),      64 >= len > 32
      // Read and write DR, 1 argument  uint64_t (uint64_t data) => (len and endState are global),      64 >= len > 32
      // Write DR,          1 argument  void     (uint64_t data) => (len and endState are global),      64 >= len > 32
      //
      // Read and write IR, 2 arguments uint64_t (uint32_t len, uint64_t data) => (endState is global), 64 >= len > 32
      // Write IR,          2 arguments void     (uint32_t len, uint64_t data) => (endState is global), 64 >= len > 32
      // Read and write DR, 2 arguments uint64_t (uint32_t len, uint64_t data) => (endState is global), 64 >= len > 32
      // Write DR,          2 arguments void     (uint32_t len, uint64_t data) => (endState is global), 64 >= len > 32

      // 3 argument scans shouldn't be needed as changing the Opcode len on each scan is unlikely
      // and even if it would happen, the existing commands can achieve the same with just 1 word overhead

      last_enum
    };


    constexpr uint32_t api_e_size = static_cast<uint32_t>(command_e::last_enum);

    static_assert((api_e_size + (1u << 4u))<= 256u, "All API calls need to leave enough space for 4-bits (16 combinations) of SCAN commands");


  }
}

#ifdef __cplusplus
}
#endif

#endif /* SRC_JTAG_API_HPP_ */
