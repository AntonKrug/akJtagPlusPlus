/*
 * jtag_global.h
 *
 *  Created on: May 30, 2021
 *      Author: anton.krug@gmail.com
 *     License: GPLv2
 */

#ifndef SRC_JTAG_JTAG_GLOBAL_H_
#define SRC_JTAG_JTAG_GLOBAL_H_

#include "main.h"


#define JTAG_TAP_TELEMETRY // Comment-out to disable the TAP state machine telemetry

#define JTAG_SHIFT_TIMMING // Comment-out to disable the bitbang shifting time LED output (can be used on the scope to count time spent bit-banging)
#ifdef JTAG_SHIFT_TIMMING
#define JTAG_SHIFT_TIMMING_PORT LD3_GPIO_Port
#define JTAG_SHIFT_TIMMING_PIN  LD3_Pin
#define JTAG_SHIFT_TIMMING_START() HAL_GPIO_WritePin(JTAG_SHIFT_TIMMING_PORT, JTAG_SHIFT_TIMMING_PIN, GPIO_PIN_SET);
#define JTAG_SHIFT_TIMMING_END()   HAL_GPIO_WritePin(JTAG_SHIFT_TIMMING_PORT, JTAG_SHIFT_TIMMING_PIN, GPIO_PIN_RESET);
#else
#define JTAG_SHIFT_TIMMING_START()
#define JTAG_SHIFT_TIMMING_END()
#endif


#endif /* SRC_JTAG_JTAG_GLOBAL_H_ */
