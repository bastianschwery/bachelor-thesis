/**
 * @author  Schwery Bastian
 * @file    dataService.h   
 * @date    06/2021
 * 
 * @brief   This class implements a rx/tx service
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <soc.h>
#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>


/*
 * defines
 * service / characteristics UUID's
 */
#define DATA_SERVICE_UUID 0xd4, 0x86, 0x48, 0x24, 0x54, 0xB3, 0x43, 0xA1, \
			            0xBC, 0x20, 0x97, 0x8F, 0xC3, 0x76, 0xC2, 0x75

#define RX_CHARACTERISTIC_UUID  0xA6, 0xE8, 0xC4, 0x60, 0x7E, 0xAA, 0x41, 0x6B, \
			                    0x95, 0xD4, 0x9D, 0xCC, 0x08, 0x4F, 0xCF, 0x6A

#define TX_CHARACTERISTIC_UUID  0xED, 0xAA, 0x20, 0x11, 0x92, 0xE7, 0x43, 0x5A, \
			                    0xAA, 0xE9, 0x94, 0x43, 0x35, 0x6A, 0xD4, 0xD3

/** @brief Callback type for when new data is received */
typedef void (*data_rx_cb_t)(uint8_t *data, uint8_t length);

/** @brief Callback struct used by the data_service Service */
struct data_service_cb 
{
	/** Data received callback. */
	data_rx_cb_t    data_rx_cb;
};

/** @brief initialize service 
 *  @return error code
*/
int data_service_init(void);

/** @brief  send data to the device given by connection parameter
 * @param conn connection to send the data
 * @param data the data to send
 * @param len length of the data to send
*/
void data_service_send(struct bt_conn *conn, const uint8_t *data, uint16_t len);

/** @brief get the diameter value
 *  @return the diameter value in cm
*/
double getDiameter();

/** @brief get number of addresses the user selected
 *  @return number of addresses
*/
uint8_t getNbrOfAddresses();

/** @brief save the address with the corresponing number at the given array
 * @param outArray the address to save the searched array
 * @param nbr the nbr of the array to save
*/
void getAddress(char* outArray, uint8_t nbr);