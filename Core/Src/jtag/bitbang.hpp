/*
 * bitbang.hpp
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 */

#ifndef SRC_JTAG_BITBANG_HPP_
#define SRC_JTAG_BITBANG_HPP_

#ifdef __cplusplus
extern "C" {
#endif

#include "tap.hpp"

namespace jtag {
  namespace bitbang {

    void shiftTms(jtag::tap::tmsMove move);

    void shiftTmsRaw(uint32_t length, uint32_t write_value);

    uint32_t shiftTdi(uint32_t length, uint32_t write_value);

    void resetTarget(uint8_t length);

    void resetTargetRelease(uint8_t length);

  }
}


#ifdef __cplusplus
}
#endif

#endif /* SRC_JTAG_BITBANG_HPP_ */
