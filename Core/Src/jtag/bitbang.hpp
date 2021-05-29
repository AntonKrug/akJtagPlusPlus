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

    void demo(void);

  }
}


#ifdef __cplusplus
}
#endif

#endif /* SRC_JTAG_BITBANG_HPP_ */
