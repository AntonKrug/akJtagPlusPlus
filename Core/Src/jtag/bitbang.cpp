/*
 * JTAG bit-bang implementation
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


    template<uint8_t WHAT_SIGNAL, uint8_t nTRSTvalue>
    __attribute__((optimize("-Ofast")))
    uint32_t shiftAsmUltraSpeed(const uint32_t length, uint32_t writeValue) {
      // This has 9.363MHz TCK at 50% duty cycle (removing the NOPs below can make it slightly faster and with duty 48% or below)
      uint32_t addressWrite = GPIOE_BASE + 0x14; // ODR register of GPIO port E
      uint32_t addressRead  = GPIOE_BASE + 0x10; // IDR register of GPIO port E

      // Break down the ODR register address calculation for the GPIO port E
      // GPIOE->ODR => GPIOE_BASE + 0x14
      //            => AHB1PERIPH_BASE + 0x1000UL + 0x14
      //            => PERIPH_BASE + 0x00020000UL + 0x1000UL + 0x14
      //            => 0x40000000 + 0x00020000UL + 0x1000UL + 0x14
      //            => 0x40021014 (GPIOE->ODR will be resolved to 0x40021014)

      uint32_t writeMask    = (1 << WHAT_SIGNAL);
      uint32_t readMask     = (1 << 31); // Masking the 31th (MSB) bit as we are shifting it already
      uint32_t count        = length;    // Counting how many bits are processed. Starting from 1 up to 'length' (inclusive) value. Set here to 0, but the code will increment it to 1 before the first check
      uint32_t outValue     = 0;         // Internal register to write values into the GPIO (driven by writeValue, WHAT_SIGNAL and nTRSTvalue)
      uint32_t outValueTck  = 0;         // Internal register to hold outValue + TCK high, without overriding the original outValue
      uint32_t inValue      = 0;         // Internal register to read raw values from GPIO and then masked/shifted correctly into the retValue
      uint32_t retValue     = 0;         // Output variable returning content from the TDI pin (driven from the inValue)

      // Normal flow (in-order)
      // - Calculate low TCK value and push it
      // - Calculate high TCK value and push it
      // - Read input TDO pin and shift it to the return register (push it from MSB as it needs to be reversed)
      // - Keep looping until finished (count the counter do branching which takes cycles)

      // Because the high part of TCK takes too long (even the calculation of low TCK happens in the high TCK,
      // before it's being pushed as low TCK) and to make 50% duty cycle the low TCK would have to be slow down.
      // Instead a lot of parts are happening out of order just so the high TCK spends as little time as possible.
      // This means that few things are calculated a iteration ahead, or iteration after
      asm volatile (
        // Pre-load output register before we start the loop
        "and.w   %[outValue],    %[writeMask],      %[writeValue], ror %[writeShiftRight]  \n\t"  // outValue = (writeValue << TDI/TMS) &  (1 << TDI/TMS-Offset)
        "orr.w   %[outValue],    %[outValue],       %[resetValue]                          \n\t"  // outValue = outValue | (nRSTvlaue << nRST)

        "cpsid if                                                                          \n\t"  // Disable IRQ temporary for critical moment

        "repeatForEachBit%=:                                                               \n\t"

        // Low part of the TCK
        "str.w   %[outValue],    [%[gpioOutAddr]]                                          \n\t"  // GPIO = outValue

        // On first cycle this is redundant, as it processed the inValue from the previous iteration
        // The first iteration is safe to do extraneously as it's just doing zeros
        "and.w   %[inValue],     %[readMask],       %[inValue],    ror %[readShift]        \n\t"  // inValue = (inValue << (from TDO-bit to 31th-bit)) & ( 1 << 31)
        "orr.w   %[retValue],    %[inValue],        %[retValue],   lsr #1                  \n\t"  // retValue = (retValue >> 1) | inValue

        // Prepare things that are needed toward the end of the loop, but can be executed now
        "orr.w   %[outValueTck], %[outValue],       %[clock_mask]                          \n\t"  // outValue = outValue | (1 << TCK) - setting TCK high
        "lsr.w   %[writeValue],  %[writeValue],     #1                                     \n\t"  // writeValue = writeValue >> 1

        // Prepare outvalue for the next iteration
        "and.w   %[outValue],    %[writeMask],      %[writeValue], ror %[writeShiftRight]  \n\t"  // outValue = (writeValue << TDI/TMS) &  (1 << TDI/TMS-Offset)
        "orr.w   %[outValue],    %[outValue],       %[resetValue]                          \n\t"  // outValue = outValue | (nRSTvlaue << nRST)


        // High part of the TCK + sample
        "str.w   %[outValueTck], [%[gpioOutAddr]]                                          \n\t"  // GPIO = outValue
        "subs.w  %[count],       #1                                                        \n\t"  // count--
        "nop                                                                               \n\t"  // balancing the high part of TCK to be 50% duty cycle
        "ldr.w   %[inValue],     [%[gpioInAddr]]                                           \n\t"  // inValue = GPIO
        "bne     repeatForEachBit%=                                                        \n\t"  // if (count != 0) then  repeatForEachBit

        "cpsie if                                                                          \n\t"  // Enable IRQ, the critical section finished

        // Process the last inValue as normally it's done in the next iteration of the loop but here we are finished with the loop
        "and.w   %[inValue],     %[readMask],       %[inValue],    ror %[readShift]        \n\t"  // inValue = (inValue << (from TDO-bit to 31th-bit)) & ( 1 << 31)
        "orr.w   %[retValue],    %[inValue],        %[retValue],   lsr #1                  \n\t"  // retValue = (retValue >> 1) | inValue


        // Outputs
        : [retValue]        "+r"(retValue),
          [count]           "+r"(count),
          [outValue]        "+r"(outValue),
          [outValueTck]     "+r"(outValueTck),
          [inValue]         "+r"(inValue),
          [writeValue]      "+r"(writeValue)

        // Inputs
        : [gpioOutAddr]     "r"(addressWrite),
          [gpioInAddr]      "r"(addressRead),
          [writeMask]       "r"(writeMask),
          [writeShiftRight] "M"(32-WHAT_SIGNAL),  // Shifting to left can be achieved by 32-N shifting to right
          [readShift]       "M"(PIN_E_TDO + 1),   // Shifting to left TDO bit to the 31th (MSB) bit can be achieved with TDO + 1 shift to the right
          [readMask]        "r"(readMask),        // Masking the 31th (MSB) bit as we are shifting it already
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

