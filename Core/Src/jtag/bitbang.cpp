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

    const uint8_t PIN_E_TMS   = 2;  // Test Mode Select
    const uint8_t PIN_E_TCK   = 3;  // Test Clock
    const uint8_t PIN_E_TDI   = 4;  // Test Data In  (from the TAP perspective). Writing to the target   (from host perspective)
    const uint8_t PIN_E_nTRST = 5;  // negated TAP Reset
    const uint8_t PIN_E_TDO   = 6;  // Test Data Out (from the TAP perspective). Reading from the target (from host perspective)

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
    uint32_t shiftAsmUltraSpeed(const uint32_t length, uint32_t writeValue) {
      // This has 9.363MHz TCK at 50% duty cycle (removing the NOPs below can make it slightly faster and with duty 48% or below)
      uint32_t addressWrite = GPIOE_BASE + 0x14; // ODR register of GPIO port E
      uint32_t addressRead  = GPIOE_BASE + 0x10; // IDR register of GPIO port E

      uint32_t writeMask = (1 << WHAT_SIGNAL);
      uint32_t count     = length;  // Counting how many bits are processed. Starting from 1 up to 'length' (inclusive) value. Set here to 0, but the code will increment it to 1 before the first check
      uint32_t outValue  = 0;       // Internal register to write values into the GPIO (driven by writeValue, WHAT_SIGNAL, PIN_E_TCK and nTRSTvalue)
      uint32_t inValue   = 0;       // Internal register to read raw values from GPIO and then masked/shifted correctly into the retValue
      uint32_t retValue  = 0;       // Output variable returning content from the TDI pin (driven from the inValue)

      asm volatile (
        "cpsid if                                                                        \n\t"  // Disable IRQ temporary for critical moment

        "repeatForEachBit%=:                                                             \n\t"

        // Low part of the TCK
        "and   %[outValue],    %[writeMask],      %[writeValue], lsr %[writeShiftRight]  \n\t"  // outValue = (writeValue << TDO/TMS) &  (1 << TDO/TMS-Offset)
        "orr   %[outValue],    %[outValue],       %[resetValue]                          \n\t"  // outValue = outValue | (nRSTvlaue << nRST)
        "str   %[outValue],    [%[gpioOutAddr]]                                          \n\t"  // GPIO = outValue

        // On first cycle this is redundant, as it processed the inValue from the previous iteration
        // The first iteration is safe to do extraneously as it's just doing zeros
        "and   %[inValue],     %[inValue],        %[readMask]                            \n\t"  // inValue = inValue & ( 1 << TDI)
        "lsl   %[inValue],     %[inValue],        %[readShift]                           \n\t"  // inValue = inValue << (pin # of TDI)
        "orr   %[retValue],    %[inValue],        %[retValue],   lsr #1                  \n\t"  // retValue = (retValue >> 1) | inValue

        // Prepare things that are needed toward the end of the loop, but can be done now
        "orr   %[outValue],    %[outValue],       %[clock_mask]                          \n\t"  // outValue = outValue | (1 << TCK) - setting TCK high
        "lsr   %[writeValue],  %[writeValue],     #1                                     \n\t"  // writeValue = writeValue >> 1
        "subs  %[count],       #1                                                        \n\t"  // count--

        // High part of the TCK + sample
        "str   %[outValue],    [%[gpioOutAddr]]                                          \n\t"  // GPIO = outValue
        "ldr   %[inValue],     [%[gpioInAddr]]                                           \n\t"  // inValue = GPIO
        "bne.n repeatForEachBit%=                                                        \n\t"  // if (count != 0) then  repeatForEachBit

        "cpsie if                                                                        \n\t"  // Enable IRQ, the critical section finished

        // Process the inValue as normally it's done in the next iteration of the loop
        "and   %[inValue],     %[inValue],        %[readMask]                            \n\t"  // inValue = inValue & ( 1 << TDI)
        "lsl   %[inValue],     %[inValue],        %[readShift]                           \n\t"  // inValue = inValue << (pin # of TDI)
        "orr   %[retValue],    %[inValue],        %[retValue],   lsr #1                  \n\t"  // retValue = (retValue >> 1) | inValue


        // Outputs
        : [retValue]        "+r"(retValue),
          [count]           "+r"(count),
          [outValue]        "+r"(outValue),
          [inValue]         "+r"(inValue)

        // Inputs
        : [gpioOutAddr]     "r"(addressWrite),
          [gpioInAddr]      "r"(addressRead),
          [writeMask]       "r"(writeMask),
          [writeValue]      "r"(writeValue),  // only input?
          [writeShiftRight] "M"(32-WHAT_SIGNAL),
          [readShift]       "M"(31-PIN_E_TDO),
          [readMask]        "I"(powerOfTwo<PIN_E_TDO>()),
          [clock_mask]      "I"(powerOfTwo<PIN_E_TCK>()),
          [resetValue]      "I"(nTRSTvalue << PIN_E_nTRST)

        // Clobbers
        : "memory"
      );

      // Shift the rest of bits as they were pushed from opposite direction
      retValue = retValue >> (32 - length);

      return retValue;
    }


    void shiftTms(tap::tmsMove move) {
      JTAG_SHIFT_TIMMING_START();
      shiftAsmUltraSpeed<PIN_E_TMS, 1>(move.amountOfBitsToShift, move.valueToShift);
      JTAG_SHIFT_TIMMING_END();
    }


    void shiftTmsRaw(uint32_t length, uint32_t writeValue) {
      JTAG_SHIFT_TIMMING_START();
      shiftAsmUltraSpeed<PIN_E_TMS, 1>(length, writeValue);
      JTAG_SHIFT_TIMMING_END();
    }


    uint32_t shiftTdi(uint32_t length, uint32_t writeValue) {
      JTAG_SHIFT_TIMMING_START();
      return shiftAsmUltraSpeed<PIN_E_TDI, 1>(length, writeValue);
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

