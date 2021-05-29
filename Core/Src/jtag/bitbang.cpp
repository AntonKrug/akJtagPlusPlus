/*
 * data.cpp
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 */

#include <cstdint>
#include <array>

#include "stm32f429xx.h"

namespace jtag {
  namespace bitbang {

    const uint8_t JTAG_TCK = 2;
    const uint8_t JTAG_TMS = 3;
    const uint8_t JTAG_TDI = 4;
    const uint8_t JTAG_TDO = 5;

    template<uint8_t WHAT_SIGNAL>
    __attribute__((optimize("-Ofast")))
    void shift(uint32_t number, const uint32_t len) {
      for (uint32_t i=0; i<len; i++) {
        GPIOE->ODR = ((number & 1) << WHAT_SIGNAL);
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        GPIOE->ODR = ((number & 1) << WHAT_SIGNAL) | (1 << JTAG_TCK);
        asm("nop");
        number = number >> 1;
      }
    }



#ifdef __cplusplus
extern "C" {
#endif

    // Test demo
    void demo() {
      for (int number = 0; number < 256; number++) {
        shift<JTAG_TDI>(number, 32);
      }
    }

#ifdef __cplusplus
}
#endif
  }
}

