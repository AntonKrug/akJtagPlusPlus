/*
 * usb.hpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */

#ifndef SRC_JTAG_USB_HPP_
#define SRC_JTAG_USB_HPP_

#include <cstdint>

#include "jtag_global.h"

// Black magic, abusing 64-bit type to transport efficiently pair of 32-bit values
// be careful of using the same order, then the input R0+R1 can just be transmitted as return R0+R1
// directly without using single instruction
// Any other method doesn't reach this efficiency, tried so far these methods:
// - Pointer to pointers arguments (extra memory reads just to read the derefenced value)
// - Global variables (will not be kept as registers and will often require memory access, which are not just 2x or more
//   slower compared to regular instructions, but having the pointer as argument is already keeping it in the register,
//   so not even 1 instruction is needed, register argument vs global then is 0 clocks vs 2-3 clocks)
// - returning structures (will cause using stack even when ABI and R0 and R1 are sufficient enough to return pair of uint32_t structure
// - partially returning one pointer and calculating offset of the second one (extra accesses to the lookup table
//   and calculating the pointer for the second time (once inside the command function, once in the parent loop)
// Usually all these methods didn't connected the connection that the input R0 and R1 are same as return R0 and R1, this
// means that toolchain often saved the results to R4/R5 just to do increment and then to move it back to R1,
// even if it could do these in place of R0/R1.
//
// This is the only case where the toolchain recognizes this connection, uses the registers in place and uses
// less extra registers (even less stuff needed to PUSH/POP), making this very efficient. Normally I'm not fan
// of these black magic solutions, but it's in a place where I care about every single instruction.
// This works extremely efficiently compared to any other approaches.
//
// The decomposition trick not just produces no extra instructions, but produces less instructions than
// when it's not used. Because when the decomposition is done is the moment when the 'toolchain' makes the click
// and recognizes what it's done here and fully optimizes this approach.
#define JTAG_COMBINE_REQ_RES(a,b)  ((uint32_t)(a) | (long long)(b) << 32)

#define JTAG_DECOMPOSE_REQ_RES(a,b,c) \
    (b) = (uint32_t *)(a); \
    (c) = (uint32_t *)((a) >> 32);


namespace jtag {

  namespace usb {


    typedef long long requestAndResponse;
    typedef requestAndResponse (*commandHandler)(uint32_t *bufRequest, uint32_t *bufResponse);

    enum class api_e:uint32_t {
      end_processing, // do not process any other command from the buffer

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

      scanIrRw1,      // Read and write IR, 1 argument  (data) => (len and endState are inferred)
      scanIr1,        // Write IR,          1 argument  (data) => (len and endState are inferred)
      scanDrRw1,      // Read and write DR, 1 argument  (data) => (len and endState are inferred)
      scanDr1,        // Write DR,          1 argument  (data) => (len and endState are inferred)

      scanIrRw2,      // Read and write IR, 2 arguments (endState, data) => (len is inferred)
      scanIr2,        // Write IR,          2 arguments (endState, data) => (len is inferred)
      scanDrRw2,      // Read and write DR, 2 arguments (endState, data) => (len is inferred)
      scanDr2,        // Write DR,          2 arguments (endState, data) => (len is inferred)

      // 3 argument scans shouldn't be needed as changing the Opcode len on each scan is unlikely
      // and even if it would happen, the existing commands can achieve the same with just 1 word overhead

      last_enum
    };

    constexpr uint32_t api_e_size = static_cast<uint32_t>(api_e::last_enum);


    void parseQueue(uint32_t *req, uint32_t *res);

  }
}


#endif /* SRC_JTAG_USB_HPP_ */
