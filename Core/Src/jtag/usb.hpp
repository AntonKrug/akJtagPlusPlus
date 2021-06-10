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

namespace jtag {

  namespace usb {


    struct commandHandler_s {
      void (*fun_ptr)(uint32_t **bufRequest, uint32_t **bufResponse);
    };


    // If the terminationValue is set to 0 and used in handleRquest, then
    // there is no need to check for buffer overflows. Because the command
    // handlers only increment the buffer as much as they process, the API
    // has dedidcated command ID 0 to end processing and the handleQueue
    // will finish processing naturally without doing extra checks.
    struct requestBuffer_s {
      uint32_t buf[JTAG_USB_REPORT_SIZE];
      uint32_t terminationValue;
    };

    // No need to check for overflows and no need to have termination,
    // each single command consumes equal or more bytes from the request buffer
    // than it can produce into the response buffer.
    struct responseBuffer_s {
      uint32_t buf[JTAG_USB_REPORT_SIZE];
    };

    enum class api_e:uint32_t {
      end_processing, // do not process any other command from the buffer

      ping,           // respond back what version this FW is

      reset,          // trst or srst
      setLed,
      setSpeed,       // set TCK speed
      getSpeed,       // get TCK speed

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


  }
}


#endif /* SRC_JTAG_USB_HPP_ */
