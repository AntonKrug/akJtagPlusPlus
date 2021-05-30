/*
 * data.cpp
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 */

#include <cstdint>
#include <array>

#include "stm32f429xx.h"
#include "bitbang.hpp"

namespace jtag {

  namespace bitbang {

    const uint8_t TCK = 2;
    const uint8_t TMS = 3;
    const uint8_t TDI = 4;
    const uint8_t TDO = 5;

    template<uint8_t WHAT_SIGNAL>
    __attribute__((optimize("-Ofast")))
    void shift(const uint32_t len, uint32_t number) {
      for (uint32_t i=0; i<len; i++) {
        GPIOE->ODR = ((number & 1) << WHAT_SIGNAL);
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        GPIOE->ODR = ((number & 1) << WHAT_SIGNAL) | (1 << TCK);
        asm("nop");
        number = number >> 1;
      }
    }


    constexpr uint8_t powerOfTwo(uint8_t number) {
        int ret = 1;
        for (int i=0; i<number; i++) {
            ret *= 2;
        }
        return ret;
    }

    // GPIOE->ODR => GPIOE_BASE + 0x14
    // AHB1PERIPH_BASE + 0x1000UL + 0x14
    // PERIPH_BASE + 0x00020000UL + 0x1000UL + 0x14 => 0x21014
    //
    template<uint8_t WHAT_SIGNAL>
    __attribute__((optimize("-Ofast")))
    void shiftAsm(const uint32_t lenght, uint32_t write_value) {
      volatile uint32_t addressWrite = GPIOE_BASE + 0x14;
      uint32_t count        = 0;
      uint32_t read_value;
      uint32_t value_shifted;
      asm volatile (
        //   "mov %[read_value],  10                             \n\t"
        "repeatForEachBit%=:                                   \n\t"

        // Low part of the TCK
        "and %[value_shifted], %[write_value],   #1            \n\t"  // value_shifted = value_shifted & pin_mask
        "lsl %[value_shifted], %[value_shifted], %[pin_shift]  \n\t"  // value_shifted = value_shifted << pin_shift
        "str %[value_shifted], [%[gpio_out_addr]]              \n\t"  // GPIO = value_shifted
        "nop                                                   \n\t"
        "nop                                                   \n\t"
        "nop                                                   \n\t"
        "nop                                                   \n\t"
        "nop                                                   \n\t"
        "nop                                                   \n\t"
        "nop                                                   \n\t"

        // High part of the TCK
        "orr %[value_shifted], %[value_shifted], %[clock_mask] \n\t"  // value_shifted = value_shifted | TCK
        "str %[value_shifted], [%[gpio_out_addr]]              \n\t"  // GPIO = value_shifted
        "nop                                                   \n\t"
        "lsr %[write_value],   %[write_value],   #1            \n\t"  // write_value = write_value >> 1
        "add %[count],         %[count],         #1            \n\t"  // count++
        "cmp %[count],         %[lenght]                       \n\t"  // if (count != lenght) then
        "bne repeatForEachBit%=                                \n\t"  //   repeatForEachBit

        // Outputs
        : [read_value]      "=r"(read_value),
          [count]           "+r"(count),
          [value_shifted]   "=r"(value_shifted)

        // Inputs
        : [gpio_out_addr]   "r"(addressWrite),
          [lenght]          "r"(lenght),
          [write_value]     "r"(write_value),
          [pin_shift]       "M"(WHAT_SIGNAL),
          [clock_mask]      "I"(powerOfTwo(TCK))

        // Clobbers
        : "memory"
      );
    }


    void shiftTms(jtag::tap::tmsMove move) {
      shiftAsm<TMS>(move.amountOfBitsToShift, move.valueToShift);
    }



#ifdef __cplusplus
extern "C" {
#endif

    // Test demo
    void demo() {
      jtag::tap::reset();
//      for (int number = 0; number < jtag::tap::tapStateSize; number++) {
//        jtag::tap::stateMove(static_cast<jtag::tap::state_e>(number));
////        shiftTms({8, 0b11111111});
////        shift<TMS>(32, number);
//      }

      jtag::tap::stateMove(jtag::tap::state_e::RunTestIdle);
      jtag::tap::stateMove(jtag::tap::state_e::UpdateIr);
    }

#ifdef __cplusplus
}
#endif
  }
}

