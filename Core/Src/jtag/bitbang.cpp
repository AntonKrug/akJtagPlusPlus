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

    // Serializing the data with clock included
    template<uint32_t N, uint8_t CHUNK_SIZE>
    __attribute__((optimize("-Ofast")))
    int shiftTdi()  {
        uint32_t shift = N;
        for (int i=0; i<CHUNK_SIZE; i++) {
            GPIOE->ODR = ((shift & 1) << JTAG_TDI);
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            GPIOE->ODR = ((shift & 1) << JTAG_TDI) | (1 << JTAG_TCK);
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            shift = shift >> 1;
        }
        return N;
    }

    // Generating function table for each input
    const int shiftTdiTableSize = 256;

    template <int lookupIndex>
    constexpr std::array<int(*)(), shiftTdiTableSize> populateShitTdiTableSize() {
        auto result = populateShitTdiTableSize<lookupIndex + 1>();
        result[lookupIndex] = shiftTdi<lookupIndex, 8>;
        return result;
    }


    template <>
    constexpr std::array<int(*)(), shiftTdiTableSize> populateShitTdiTableSize<shiftTdiTableSize>() {
        std::array<int(*)(), shiftTdiTableSize> lookupTable = { 0 };
        return lookupTable;
    }


    const auto shiftFunctions = populateShitTdiTableSize<0>();

#ifdef __cplusplus
extern "C" {
#endif

    // Test demo
    void demo() {
      for (int number = 0; number < 256; number++) {
        shiftFunctions[number]();
      }
    }

#ifdef __cplusplus
}
#endif
  }
}

