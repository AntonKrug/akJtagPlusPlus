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

      // TODO: Write this as inline assembly, just to guarantee the 50% duty cycle between the two writes
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


    // GPIOE_BASE + 0x14
    // AHB1PERIPH_BASE + 0x1000UL + 0x14
    // PERIPH_BASE + 0x00020000UL + 0x1000UL + 0x14 => 0x21014
    //
    void shiftAsm(const uint32_t lenght, uint32_t value) {
      uint32_t read_value;
      asm (
//          "mov %[count], #0 \n\t"
//          "mov %[read_value], #10              \n\t"
          "mov %[read_value], 10   \n\t"
          "mov ip, #0 \n\t"
          "add ip, %[gpio_out_addr_high]   \n\t"
          "lsl ip, ip, #8 \n\t"
          "add ip, %[gpio_out_addr_mid]   \n\t"
          "lsl ip, ip, #8 \n\t"
          "add ip, %[gpio_out_addr_low]   \n\t"
          "LSL %[read_value], %[read_value], %[pin_shift]"
//          "mov ip,            %[gpio_out_addr] \n\t"
          : [read_value]        "=r"(read_value)
          : [gpio_out_addr_low]  "I"((0x21014) & 0xff),
            [gpio_out_addr_mid]  "I"((0x21014) & 0xff00 >> 8),
            [gpio_out_addr_high] "I"((0x21014) & 0xff0000 >> 16),
            [pin_shift]          "M"(2)
      );
//        "sample%=:                  \n\t"
//        "in %[reg0],   %[addr]      \n\t"
//        "in %[reg1],   %[addr]      \n\t"
//        "in %[reg2],   %[addr]      \n\t"
//        "in %[reg3],   %[addr]      \n\t"
//        "inc %[major]               \n\t" // Increment the major counter
//        "in %[reg4],   %[addr]      \n\t"
//        "in %[reg5],   %[addr]      \n\t"
//        "in %[reg6],   %[addr]      \n\t"
//        "in %[reg7],   %[addr]      \n\t"
//        "cpi %[major], %[major_max] \n\t" // Compare major counter with SINGLE_PIN_CAPACITIVE_SENSE_TIMEOUT
//        "in %[reg8],   %[addr]      \n\t"
//        "in %[reg9],   %[addr]      \n\t"
//        "in %[reg10],  %[addr]      \n\t"
//        "in %[reg11],  %[addr]      \n\t"
//        "breq timeout%=             \n\t" // Branch if equal (major == SINGLE_PIN_CAPACITIVE_SENSE_TIMEOUT)
//        "in %[reg12],  %[addr]      \n\t"
//        "in %[reg13],  %[addr]      \n\t"
//        "in %[reg14],  %[addr]      \n\t"
//        "sbis %[addr], %[bit]       \n\t" // Skip if bit in I/O is set, no need to read the sample into a register
//        "rjmp sample%=              \n\t"
//      );
    }

    void shiftTms(jtag::tap::tmsMove move) {
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

