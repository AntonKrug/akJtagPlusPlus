/*
 * usb.hpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */

#ifndef SRC_JTAG_USB_HPP_
#define SRC_JTAG_USB_HPP_


namespace jtag {

  namespace usb {

    enum class api_e:uint32_t {
      ping,
      reset,
      setLed,
      setSpeed,
      getSpeed,
      stateMove,     // Specify endState after commands (don't change TAP SM)
      pathMove,      // move current state to the endstate (change TAP SM)
      runTest,
      setIrOpcodeLen,
      setDrOpcodeLen,
      scanIrRw2,
      scanIr2,
      scanDrRw2,
      scanDr2,
      scanIrRw1,
      scanIr1,
      scanDrRw1,
      scanDr1,
    }
  }
}


#endif /* SRC_JTAG_USB_HPP_ */
