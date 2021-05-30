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
    uint32_t shift(const uint32_t len, uint32_t number) {
      uint32_t ret = 0;
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
        ret = ret << 1 | ( (GPIOE->IDR >> TDI) & 0x1);
        asm("nop");
        number = number >> 1;
      }
      return ret;
    }


    constexpr uint8_t powerOfTwo(uint8_t number) {
        int ret = 1;
        for (int i=0; i<number; i++) {
            ret *= 2;
        }
        return ret;
    }

    // GPIOE->ODR => GPIOE_BASE + 0x14
    // GPIOE->ODR => AHB1PERIPH_BASE + 0x1000UL + 0x14
    // GPIOE->ODR => PERIPH_BASE + 0x00020000UL + 0x1000UL + 0x14
    // GPIOE->ODR => 0x40000000 + 0x00020000UL + 0x1000UL + 0x14 => 0x40021014
    template<uint8_t WHAT_SIGNAL>
    __attribute__((optimize("-Ofast")))
    uint32_t shiftAsm(const uint32_t lenght, uint32_t write_value) {
      volatile uint32_t addressWrite = GPIOE_BASE + 0x14; // ODR register of GPIO port E
      volatile uint32_t addressRead  = GPIOE_BASE + 0x10; // IDR register of GPIO port E

      uint32_t count = 0;
      uint32_t value_shifted = 0;
      uint32_t ret_value = 0;

      asm volatile (
        //   "mov %[read_value],  10                               \n\t"
        "cpsid if                                                  \n\t"  // Disable IRQ
        "repeatForEachBit%=:                                       \n\t"

        // Low part of the TCK
        "and.w %[value_shifted], %[write_value],    #1             \n\t"  // value_shifted = write_value & 1
        "lsls  %[value_shifted], %[value_shifted],  %[write_shift] \n\t"  // value_shifted = value_shifted << pin_shift
        "str   %[value_shifted], [%[gpio_out_addr]]                \n\t"  // GPIO = value_shifted
        "nop                                                       \n\t"
        "nop                                                       \n\t"
        "nop                                                       \n\t"
        "nop                                                       \n\t"
        "nop                                                       \n\t"
        "nop                                                       \n\t"
          "nop                                                       \n\t"
          "nop                                                       \n\t"
        "orr.w %[value_shifted], %[value_shifted],  %[clock_mask]  \n\t"  // value_shifted = value_shifted | TCK
        "lsr   %[write_value],   %[write_value],    #1             \n\t"  // write_value = write_value >> 1
        "adds  %[count],         #1                                \n\t"  // count++
        "cmp   %[count],         %[lenght]                         \n\t"  // if (count != lenght) then ....


        // High part of the TCK
        "str   %[value_shifted], [%[gpio_out_addr]]                \n\t"  // GPIO = value_shifted
        "ldr   %[value_shifted], [%[gpio_in_addr]]                 \n\t"  // value_shifted = GPIO
        "lsr   %[value_shifted], %[value_shifted], %[read_shift]   \n\t"  // value_shifted = value_shifted >> TDI
        "and.w %[value_shifted], %[value_shifted], #1              \n\t"  // value_shifted = value_shifted & 1
        "lsl   %[ret_value],     #1                                \n\t"  // ret = ret << 1
        "orr.w %[ret_value],     %[ret_value], %[value_shifted]    \n\t"  // ret = ret | value_shifted
        "bne.n repeatForEachBit%=                                  \n\t"  // if (count != lenght) then  repeatForEachBit

        "cpsie if                                                  \n\t"  // Enable IRQ

        // Outputs
        : [ret_value]       "+r"(ret_value),
          [count]           "+r"(count),
          [value_shifted]   "+r"(value_shifted)

        // Inputs
        : [gpio_out_addr]   "r"(addressWrite),
          [gpio_in_addr]    "r"(addressRead),
          [lenght]          "r"(lenght),
          [write_value]     "r"(write_value),
          [write_shift]     "M"(WHAT_SIGNAL),
          [read_shift]      "M"(TDI),
          [clock_mask]      "I"(powerOfTwo(TCK))

        // Clobbers
        : "memory"
      );

      return ret_value;
    }


    void shiftTms(jtag::tap::tmsMove move) {
      shiftAsm<TMS>(move.amountOfBitsToShift, move.valueToShift);
      shift<TMS>(move.amountOfBitsToShift, move.valueToShift);
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

