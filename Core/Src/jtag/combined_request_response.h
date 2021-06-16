/*
 * combined_request_response.h
 *
 *  Created on: Jun 17, 2021
 *      Author: Fredy
 */

#ifndef SRC_JTAG_COMBINED_REQUEST_RESPONSE_H_
#define SRC_JTAG_COMBINED_REQUEST_RESPONSE_H_

#include <stdint.h>

typedef uint64_t requestAndResponse;
typedef requestAndResponse (*commandHandler)(uint32_t *bufRequest, uint32_t *bufResponse);

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
// - union of uint64_t with a struct of uint32_t pair creates overhead as well and extra memory access
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
#define JTAG_COMBINE_REQ_RES(a,b)  ((uint32_t)(a) | (uint64_t)(b) << 32)

#define JTAG_DECOMPOSE_REQ_RES(a,b,c) \
    (b) = (uint32_t *)(a); \
    (c) = (uint32_t *)((a) >> 32);


#endif /* SRC_JTAG_COMBINED_REQUEST_RESPONSE_H_ */
