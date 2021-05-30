/*
 * c_connector.h
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 *     License: GPLv2
 */

#ifndef SRC_JTAG_JTAG_C_CONNECTOR_H_
#define SRC_JTAG_JTAG_C_CONNECTOR_H_

#include "jtag_global.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef JTAG_TAP_TELEMETRY
void jtag_tap_telemetry_dispay(void);
#endif

void jtag_setup(void);

void jtag_loop(void);


#ifdef __cplusplus
}
#endif

#endif /* SRC_JTAG_JTAG_C_CONNECTOR_H_ */
