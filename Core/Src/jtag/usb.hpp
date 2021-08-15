/*
 * usb.hpp
 *
 *  Created on: Jun 10, 2021
 *      Author: anton.krug@gmail.com
 */

#ifndef SRC_JTAG_USB_HPP_
#define SRC_JTAG_USB_HPP_

#include <cstdint>

#include "jtag_global.h"

#include "combined_request_response.h"

#ifdef __cplusplus
extern "C" {
#endif


namespace jtag {

  namespace usb {

    requestAndResponse parseQueue(uint32_t *req, uint32_t *res);

  }
}

#ifdef __cplusplus
}
#endif

#endif /* SRC_JTAG_USB_HPP_ */
