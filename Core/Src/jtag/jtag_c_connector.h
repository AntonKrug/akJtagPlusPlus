/*
 * c_connector.h
 *
 *  Created on: May 28, 2021
 *      Author: anton.krug@gmail.com
 */

#ifndef SRC_JTAG_JTAG_C_CONNECTOR_H_
#define SRC_JTAG_JTAG_C_CONNECTOR_H_


#ifdef __cplusplus
extern "C" {
#endif

void jtag_setup(void);


void jtag_loop(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_JTAG_JTAG_C_CONNECTOR_H_ */
