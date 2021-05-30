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
      uint32_t addressWrite = GPIOE_BASE + 0x14; // ODR register of GPIO port E
      uint32_t addressRead  = GPIOE_BASE + 0x10; // IDR register of GPIO port E

      uint32_t count     = 0;
      uint32_t shift_out = 0;
      uint32_t shift_in  = 0;
      uint32_t ret_value = 0;

      asm volatile (
        "cpsid if                                                  \n\t"  // Disable IRQ
        "repeatForEachBit%=:                                       \n\t"

        // Low part of the TCK
        "and.w %[shift_out],   %[write_value],    #1               \n\t"  // shift_out = write_value & 1
        "lsls  %[shift_out],   %[shift_out],      %[write_shift]   \n\t"  // shift_out = shift_out << pin_shift
        "str   %[shift_out],   [%[gpio_out_addr]]                  \n\t"  // GPIO = shift_out

        // On first cycle this is redundant, as it processed the shift_in from the previous iteration
        "lsr   %[shift_in],    %[shift_in],       %[read_shift]    \n\t"  // shift_in = shift_in >> TDI
        "and.w %[shift_in],    %[shift_in],       #1               \n\t"  // shift_in = shift_in & 1
        "lsl   %[ret_value],   #1                                  \n\t"  // ret = ret << 1
        "orr.w %[ret_value],   %[ret_value],      %[shift_in]      \n\t"  // ret = ret | shift_in

        // Prepare things which are needed torward end of the loop, but can be done now
        "orr.w %[shift_out],   %[shift_out],      %[clock_mask]    \n\t"  // shift_out = shift_out | TCK
        "lsr   %[write_value], %[write_value],    #1               \n\t"  // write_value = write_value >> 1
        "adds  %[count],       #1                                  \n\t"  // count++
        "cmp   %[count],       %[lenght]                           \n\t"  // if (count != lenght) then ....

        // High part of the TCK + sample
        "str   %[shift_out],   [%[gpio_out_addr]]                  \n\t"  // GPIO = shift_out
        "nop                                                       \n\t"
        "nop                                                       \n\t"
        "ldr   %[shift_in],    [%[gpio_in_addr]]                   \n\t"  // shift_in = GPIO
        "bne.n repeatForEachBit%=                                  \n\t"  // if (count != lenght) then  repeatForEachBit

        "cpsie if                                                  \n\t"  // Enable IRQ

        // Process the shift_in as normally it's done in the next iteration of the loop
        "lsr   %[shift_in],    %[shift_in],       %[read_shift]    \n\t"  // shift_in = shift_in >> TDI
        "and.w %[shift_in],    %[shift_in],       #1               \n\t"  // shift_in = shift_in & 1
        "lsl   %[ret_value],   #1                                  \n\t"  // ret = ret << 1
        "orr.w %[ret_value],   %[ret_value],      %[shift_in]      \n\t"  // ret = ret | shift_in


        // Outputs
        : [ret_value]       "+r"(ret_value),
          [count]           "+r"(count),
          [shift_out]       "+r"(shift_out),
          [shift_in]        "+r"(shift_in)

        // Inputs
        : [gpio_out_addr]   "r"(addressWrite),
          [gpio_in_addr]    "r"(addressRead),
          [lenght]          "r"(lenght),
          [write_value]     "r"(write_value),
          [write_shift]     "M"(WHAT_SIGNAL),
          [read_shift]      "M"(TDO),
          [clock_mask]      "I"(powerOfTwo(TCK))

        // Clobbers
        : "memory"
      );

      return ret_value;
    }


    void shiftTms(jtag::tap::tmsMove move) {
      shiftAsm<TMS>(move.amountOfBitsToShift, move.valueToShift);
//      shift<TMS>(move.amountOfBitsToShift, move.valueToShift);
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

