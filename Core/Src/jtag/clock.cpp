/*
 * speed.cpp
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 */

#include <cstdint>
#include <array>

#include "stm32f429i_discovery_lcd.h"

namespace jtag {

  namespace clock {

    const int apb2Clock = 168'000'000;

    // These TIMx ARR dividers ....
    constexpr std::array dividers{4, 5, 6, 7, 8, 10, 14, 20, 42};

    constexpr int lookupTableSize = dividers.size();

    template <int lookupIndex>
    constexpr std::array<uint32_t, lookupTableSize> populateFrequenciesTable() {
        auto previousResult = populateFrequenciesTable<lookupIndex + 1>();

        previousResult[lookupIndex] = (apb2Clock / ( (dividers[lookupIndex]) * 2)) / 1'000;
        return previousResult;
    }


    template <>
    constexpr std::array<uint32_t, lookupTableSize> populateFrequenciesTable<lookupTableSize>() {
        return { 0 };
    }


    // ... will produe the following frequencies
    constexpr auto frequencies = populateFrequenciesTable<0>();


    int currentIndex = 0;

    void display() {

    }

  }

}
