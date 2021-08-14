/**
 * @file 	DataService.h
 * @author 	Schwery Bastian (bastian98@gmx.ch)
 * @brief 	This class implements a rx/tx service
 * @version 0.1
 * @date 	2021-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/*---------------------------------------------------------------------------
 * INCLUDES
 *--------------------------------------------------------------------------*/ 
#include <bluetooth/gatt.h>

/*---------------------------------------------------------------------------
 * DEFINES
 *--------------------------------------------------------------------------*/ 
#define DATA_SERVICE_UUID 0xd4, 0x86, 0x48, 0x24, 0x54, 0xB3, 0x43, 0xA1, \
			            0xBC, 0x20, 0x97, 0x8F, 0xC3, 0x76, 0xC2, 0x75

#define RX_CHARACTERISTIC_UUID  0xA6, 0xE8, 0xC4, 0x60, 0x7E, 0xAA, 0x41, 0x6B, \
			                    0x95, 0xD4, 0x9D, 0xCC, 0x08, 0x4F, 0xCF, 0x6A

#define TX_CHARACTERISTIC_UUID  0xED, 0xAA, 0x20, 0x11, 0x92, 0xE7, 0x43, 0x5A, \
			                    0xAA, 0xE9, 0x94, 0x43, 0x35, 0x6A, 0xD4, 0xD3

// declaration of the UUID's
#define BT_UUID_DATA_SERVICE      BT_UUID_DECLARE_128(DATA_SERVICE_UUID)
#define BT_UUID_DATA_SERVICE_RX   BT_UUID_DECLARE_128(RX_CHARACTERISTIC_UUID)
#define BT_UUID_DATA_SERVICE_TX   BT_UUID_DECLARE_128(TX_CHARACTERISTIC_UUID)

#define MAX_TRANSMIT_SIZE 240	

/**
 * @brief Callback type for when new data is received
 * 
 */
typedef void (*data_rx_cb_t)(uint8_t *data, uint8_t length);

/** 
 * @brief Callback struct used by the data_service Service 
 * 
*/
struct data_service_cb 
{
	// Data received callback
	data_rx_cb_t data_rx_cb;
};

/** 
 * @brief initialize service 
 * 
 *  @return uint8_t error code
*/
uint8_t data_service_init(void);

/** 
 * @brief  send data to the device given by connection parameter
 * 
 * @param conn connection to send the data
 * @param data the data to send
 * @param len length of the data to send
*/
void data_service_send(struct bt_conn *conn, const uint8_t *data, uint16_t len);

/** 
 *  @brief get the diameter value
 * 
 *  @return the diameter value in cm
*/
double getDiameter();

/**
 * @brief Set the diameter value
 * 
 * @param diameter value 
 */
void setDiameter(uint8_t diameter);

/** 
 *  @brief get number of addresses the user selected
 * 
 *  @return number of addresses
*/
uint8_t getNbrOfAddresses();

/** 
 * @brief save the address with the corresponing number at the given array
 * 
 * @param outArray the address to save the searched array
 * @param nbr the nbr of the array to save
*/
void getAddress(char* outArray, uint8_t nbr);

/**
 * @brief Get informations about which sensors 
 * 		  the user wants to connect
 * 
 * @return uint8_t info about sensors
 * 		   just one speed sensor: 1
 *		   just one cadence sensor: 2
 *		   speed and cadence sensors: 3
 *		   speed and cadence and heart rate sensor: 4
 * 		   speed and heart rate sensor: 5
 * 		   cadence and heart rate sensor: 6
 * 		   just one heart rate sensor: 7
 */
uint8_t getSensorInfos();

/**
 * @brief get information if notifications are enabled in application
 * 
 * @return true if enabled
 * @return false if is not enabled
 */
bool areNotificationsOn();
