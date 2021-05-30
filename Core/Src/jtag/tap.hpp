/*
 * jtag.hpp
 *
 *  Created on: May 27, 2021
 *      Author: anton.krug@gmail.com
 *     License: GPLv2
 */


#ifndef SRC_JTAG_TAP_HPP_
#define SRC_JTAG_TAP_HPP_


#ifdef __cplusplus
extern "C" {
#endif

#include <cstdint>
#include "jtag_global.h"

namespace jtag {

  namespace tap {


    // https://image.slidesharecdn.com/jtagpresentation-100723072934-phpapp01/95/jtag-presentation-17-728.jpg?cb=1279870813
    enum class state_e:uint32_t {
      TestLogicReset,
      RunTestIdle,
      SelectDrScan,
      CaptureDr,
      ShiftDr,
      Exit1Dr,
      PauseDr,
      Exit2Dr,
      UpdateDr,
      SelectIrScan,
      CaptureIr,
      ShiftIr,
      Exit1Ir,
      PauseIr,
      Exit2Ir,
      UpdateIr,
      LAST_ENUM
    };

    const int tapStateSize = static_cast<int>(state_e::LAST_ENUM);

    extern state_e currentState;

    struct tmsMove {
      uint8_t amountOfBitsToShift;
      uint8_t valueToShift;
    };


    void reset(void);
    void stateMove(state_e whereToMove);

#ifdef JTAG_TAP_TELEMETRY
    namespace telemetry {
      void displayStateMachineDiagram(void);
    }
#endif


  }

}


#ifdef __cplusplus
}
#endif

#endif /* SRC_JTAG_TAP_HPP_ */

