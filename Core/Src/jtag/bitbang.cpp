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
    const uint8_t RST = 6;


    template<uint8_t number>
    constexpr uint8_t powerOfTwo() {
        static_assert(number <8, "Output would overflow, the JTAG pins are close to base of the register and you shouldn't need PIN8 or above anyway");
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
    uint32_t shiftAsm8(const uint32_t length, uint32_t write_value) {
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
        "lsls  %[shift_out],   %[shift_out],      %[write_shift]   \n\t"  // shift_out = shift_out << write_shift
        "str   %[shift_out],   [%[gpio_out_addr]]                  \n\t"  // GPIO = shift_out

        // On first cycle this is redundant, as it processed the shift_in from the previous iteration
        "lsr   %[shift_in],    %[shift_in],       %[read_shift]    \n\t"  // shift_in = shift_in >> (pin # of TDI)
        "and.w %[shift_in],    %[shift_in],       #1               \n\t"  // shift_in = shift_in & 1
        "lsl   %[ret_value],   #1                                  \n\t"  // ret = ret << 1
        "orr.w %[ret_value],   %[ret_value],      %[shift_in]      \n\t"  // ret = ret | shift_in

        // Prepare things that are needed toward the end of the loop, but can be done now
        "orr.w %[shift_out],   %[shift_out],      %[clock_mask]    \n\t"  // shift_out = shift_out | (1 << TCK)
        "lsr   %[write_value], %[write_value],    #1               \n\t"  // write_value = write_value >> 1
        "adds  %[count],       #1                                  \n\t"  // count++
        "cmp   %[count],       %[length]                           \n\t"  // if (count != length) then ....

        // High part of the TCK + sample
        "str   %[shift_out],   [%[gpio_out_addr]]                  \n\t"  // GPIO = shift_out
        "nop                                                       \n\t"
        "nop                                                       \n\t"
        "ldr   %[shift_in],    [%[gpio_in_addr]]                   \n\t"  // shift_in = GPIO
        "bne.n repeatForEachBit%=                                  \n\t"  // if (count != length) then  repeatForEachBit

        "cpsie if                                                  \n\t"  // Enable IRQ, the critical section finished

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
          [length]          "r"(length),
          [write_value]     "r"(write_value),
          [write_shift]     "M"(WHAT_SIGNAL),
          [read_shift]      "M"(TDO),
          [clock_mask]      "I"(powerOfTwo<TCK>())

        // Clobbers
        : "memory"
      );

      return ret_value;
    }


    void shiftTms(jtag::tap::tmsMove move) {
      shiftAsm8<TMS>(move.amountOfBitsToShift, move.valueToShift);
    }


    void shiftTms(uint32_t length, uint32_t write_value) {
      shiftAsm8<TMS>(length, write_value);
    }


    uint32_t shiftTdi(uint32_t length, uint32_t write_value) {
      return shiftAsm8<TDI>(length, write_value);
    }


  }
}

