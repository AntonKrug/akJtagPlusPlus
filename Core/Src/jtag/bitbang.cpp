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
    template <int CHUNK_SIZE>
    constexpr std::array<uint16_t, CHUNK_SIZE * 2> shift(const uint32_t input, const uint8_t pin) {
        std::array<uint16_t, CHUNK_SIZE * 2> ret = {};
        uint32_t shift = input;
        for (int i=0; i<CHUNK_SIZE; i++) {
            ret[i*2]   = (input & 0x1) << pin;
            ret[i*2+1] = (input & 0x1) << pin | (1) << JTAG_TCK;
            shift = shift >> 1;
        }
        return ret;
    }

    const auto shifted = shift<8>(128, JTAG_TDO);

    // TDI lookup generation
    const int tdiTableSize = 256;

    template <int lookupIndex>
    constexpr std::array<std::array<uint16_t, 16>, tdiTableSize> populateTdiTable() {
        auto previousResult = populateTdiTable<lookupIndex + 1>();

        previousResult[lookupIndex] = shift<8>((uint32_t)lookupIndex, JTAG_TDI);
        return previousResult;
    }


    template <>
    constexpr std::array<std::array<uint16_t, 16>, tdiTableSize> populateTdiTable<tdiTableSize>() {
        return { 0 };
    }

    constexpr auto tdiLookup = populateTdiTable<0>();

    void demo() {
      for (int number = 0; number < 256; number++) {
        for (int clock = 0; clock < 16; clock++) {
          GPIOE->ODR = tdiLookup[number][clock];
        }
      }
    }
  }

}

