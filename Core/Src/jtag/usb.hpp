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

#include "combined_request_response.h"

#ifdef __cplusplus
extern "C" {
#endif


namespace jtag {

  namespace usb {


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


    requestAndResponse parseQueue(uint32_t *req, uint32_t *res);

  }
}

#ifdef __cplusplus
}
#endif

#endif /* SRC_JTAG_USB_HPP_ */
