/*
 * data.cpp
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 *     License: GPLv2
 */

#include <cstdint>
#include <array>

#include "bitbang.hpp"

#include "main.h"
#include "jtag_global.h"

namespace jtag {

  namespace bitbang {

    const uint8_t PIN_E_TCK   = 2;  // Test Clock
    const uint8_t PIN_E_TMS   = 3;  // Test Mode Select
    const uint8_t PIN_E_TDI   = 4;  // Test Data In  (from the TAP perspective). Writing to the target   (from host perspective)
    const uint8_t PIN_E_TDO   = 5;  // Test Data Out (from the TAP perspective). Reading from the target (from host perspective)
    const uint8_t PIN_E_nTRST = 6;  // negated TAP Reset

    const uint8_t PIN_C_VJTAG = 13;
    const uint8_t PIN_C_nSRST = 14; // negated System Reset


    template<uint8_t number>
    constexpr uint8_t powerOfTwo() {
        static_assert(number <8, "Output would overflow, the JTAG pins are close to base of the register and you shouldn't need PIN8 or above anyway");

        return (1 << number);
    }


    // GPIOE->ODR => GPIOE_BASE + 0x14
    // GPIOE->ODR => AHB1PERIPH_BASE + 0x1000UL + 0x14
    // GPIOE->ODR => PERIPH_BASE + 0x00020000UL + 0x1000UL + 0x14
    // GPIOE->ODR => 0x40000000 + 0x00020000UL + 0x1000UL + 0x14 => 0x40021014
    template<uint8_t WHAT_SIGNAL, uint8_t nTRSTvalue>
    __attribute__((optimize("-Ofast")))
    uint32_t shiftAsmUltraSpeed(const uint32_t length, uint32_t write_value) {
      // This has 9.363MHz TCK at 50% duty cycle (removing the NOPs below can make it slightly faster and with duty 48% or below)
      uint32_t addressWrite = GPIOE_BASE + 0x14; // ODR register of GPIO port E
      uint32_t addressRead  = GPIOE_BASE + 0x10; // IDR register of GPIO port E

      uint32_t count     = 0;  // Counting how many bits are processed. Starting from 1 up to 'length' (inclusive) value. Set here to 0, but the code will increment it to 1 before the first check
      uint32_t out_value = 0;  // Internal register to write values into the GPIO (driven by write_value, WHAT_SIGNAL, PIN_E_TCK and nTRSTvalue)
      uint32_t in_value  = 0;  // Internal register to read raw values from GPIO and then masked/shifted correctly into the ret_value
      uint32_t ret_value = 0;  // Output variable returning content from the TDI pin (driven from the in_value)

      asm volatile (
        "cpsid if                                                  \n\t"  // Disable IRQ temporary for critical moment

        "repeatForEachBit%=:                                       \n\t"

        // Low part of the TCK
        "and.w %[out_value],   %[write_value],    #1               \n\t"  // out_value = write_value & 1
        "lsls  %[out_value],   %[out_value],      %[write_shift]   \n\t"  // out_value = out_value << write_shift
        "orr.w %[out_value],   %[out_value],      %[reset_value]   \n\t"  // out_value = out_value | (nRSTvlaue << nRST)
        "str   %[out_value],   [%[gpio_out_addr]]                  \n\t"  // GPIO = out_value

        // On first cycle this is redundant, as it processed the in_value from the previous iteration
        // The first iteration is safe to do extraneously as it's just doing zeros
        "and.w %[in_value],    %[in_value],       %[read_mask]     \n\t"  // in_value = in_value & ( 1 << TDI)
        "lsl   %[in_value],    %[in_value],       %[read_shift]    \n\t"  // in_value = in_value >> (pin # of TDI)
        "lsr   %[ret_value],   #1                                  \n\t"  // ret_value = ret_value >> 1
        "orr.w %[ret_value],   %[ret_value],      %[in_value]      \n\t"  // ret_value = ret_value | in_value

        // Prepare things that are needed toward the end of the loop, but can be done now
        "orr.w %[out_value],   %[out_value],      %[clock_mask]    \n\t"  // out_value = out_value | (1 << TCK) - setting TCK high
        "lsr   %[write_value], %[write_value],    #1               \n\t"  // write_value = write_value >> 1
        "adds  %[count],       #1                                  \n\t"  // count++
        "cmp   %[count],       %[length]                           \n\t"  // if (count != length) then ....

        // High part of the TCK + sample
        "str   %[out_value],   [%[gpio_out_addr]]                  \n\t"  // GPIO = out_value
        "nop                                                       \n\t"
        "ldr   %[in_value],    [%[gpio_in_addr]]                   \n\t"  // in_value = GPIO
        "bne.n repeatForEachBit%=                                  \n\t"  // if (count != length) then  repeatForEachBit

        "cpsie if                                                  \n\t"  // Enable IRQ, the critical section finished

        // Process the in_value as normally it's done in the next iteration of the loop
        "and.w %[in_value],    %[in_value],       %[read_mask]     \n\t"  // in_value = in_value & ( 1 << TDI)
        "lsl   %[in_value],    %[in_value],       %[read_shift]    \n\t"  // in_value = in_value >> (pin # of TDI)
        "lsr   %[ret_value],   #1                                  \n\t"  // ret_value = ret_value >> 1
        "orr.w %[ret_value],   %[ret_value],      %[in_value]      \n\t"  // ret_value = ret_value | in_value


        // Outputs
        : [ret_value]       "+r"(ret_value),
          [count]           "+r"(count),
          [out_value]       "+r"(out_value),
          [in_value]        "+r"(in_value)

        // Inputs
        : [gpio_out_addr]   "r"(addressWrite),
          [gpio_in_addr]    "r"(addressRead),
          [length]          "r"(length),
          [write_value]     "r"(write_value),
          [write_shift]     "M"(WHAT_SIGNAL),
          [read_shift]      "M"(31-PIN_E_TDO),
          [read_mask]       "I"(powerOfTwo<PIN_E_TDO>()),
          [clock_mask]      "I"(powerOfTwo<PIN_E_TCK>()),
          [reset_value]     "I"(nTRSTvalue << PIN_E_nTRST)

        // Clobbers
        : "memory"
      );

      // Shift the rest of bits as they were pushed from opposite direction
      ret_value = ret_value >> (32 - length);

      return ret_value;
    }


    void shiftTms(tap::tmsMove move) {
      JTAG_SHIFT_TIMMING_START();
      shiftAsmUltraSpeed<PIN_E_TMS, 1>(move.amountOfBitsToShift, move.valueToShift);
      JTAG_SHIFT_TIMMING_END();
    }


    void shiftTmsRaw(uint32_t length, uint32_t write_value) {
      JTAG_SHIFT_TIMMING_START();
      shiftAsmUltraSpeed<PIN_E_TMS, 1>(length, write_value);
      JTAG_SHIFT_TIMMING_END();
    }


    uint32_t shiftTdi(uint32_t length, uint32_t write_value) {
      JTAG_SHIFT_TIMMING_START();
      return shiftAsmUltraSpeed<PIN_E_TDI, 1>(length, write_value);
      JTAG_SHIFT_TIMMING_END();
    }


    void resetSignal(uint8_t isSrst, int8_t length) {
      // TODO: implement srst and trst
      // should do signal reset instead of the state machine reset
      // length has to be under or equal to 32

      if (length < 0) length = 32;
      // We will pull the reset low, while shifting 1s to TMS (which should put it into reset and keep it there on its own as well)
      shiftAsmUltraSpeed<PIN_E_TMS, 0>(length, 0xffff'ffff);
    }


  }
}

