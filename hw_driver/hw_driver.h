/*
 * hw_driver.h
 *
 *  Created on: Jul 23, 2026
 *      Author: YarikSherb
 */

#ifndef HW_DRIVER_HW_DRIVER_H_
#define HW_DRIVER_HW_DRIVER_H_

void hw_init(void *handler);

void transmit_data_byte(void *handler, char byte);

char IsActiveFlag_TX(void *handler);

char recive_data_byte(void *handler);

char IsActiveFlag_RX(void *handler);

unsigned int Flash_WriteBuffer(unsigned int flash_addr,
                                    const void *ram_addr,
									unsigned int size);

#endif /* HW_DRIVER_HW_DRIVER_H_ */
