/**
 * @file    deviceManager.h
 * @author  Schwery Bastian (bastian98@gmx.ch)
 * @brief   handle peripheral and central connections and 
 *          the data flow between the sensors and the android
 *          application
 * @version 0.1
 * @date    2021-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "dataCSC.h"
#include "dataService.h"

extern "C" {
    #include "BatteryManager.h"
}

#include <bluetooth/bluetooth.h>
#include <bluetooth/addr.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/hci.h>
#include <bluetooth/services/lbs.h>
#include <dk_buttons_and_leds.h>
#include <settings/settings.h>
#include <bluetooth/scan.h>

#include <bluetooth/gatt_dm.h>
#include <bluetooth/att.h>
#include <sys/byteorder.h>

/*---------------------------------------------------------------------------
 * DEFINES
 *--------------------------------------------------------------------------*/ 

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)


#define CON_STATUS_LED_PERIPHERAL       DK_LED1
#define CON_STATUS_LED_CENTRAL          DK_LED2

#define USER_LED                DK_LED4

#define USER_BUTTON             DK_BTN1_MSK

// Thingy advertisement UUID 
#define BT_UUID_THINGY                                                         \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x01, 0x68, 0xEF)

// Thingy service UUID 
#define BT_UUID_UI                                                         \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x03, 0x68, 0xEF)

// Thingy button characteristic UUID 
#define BT_UUID_BUTTON                                                     \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x02, 0x03, 0x68, 0xEF)

// Thingy led characteristic UUID 
#define BT_UUID_LED                                                        \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x01, 0x03, 0x68, 0xEF)

// Type definitions
#define TYPE_CSC_SPEED 1
#define TYPE_CSC_CADENCE 2
#define TYPE_HEARTRATE 3
#define TYPE_BATTERY 4

// define maximum of central connections
#define MAX_CONNECTIONS_CENTRAL 5

class deviceManager {
public:
/*---------------------------------------------------------------------------
 * general methods
 *--------------------------------------------------------------------------*/ 

    // constructor
    deviceManager();

   /**
    * @brief Get the Device object
    * 
    * @return uint8_t 
    *         3 when central & peripheral role
    *         2 when peripheral role
    *         1 when central role
    */
    static uint8_t getDevice();

    /**
     * @brief setter:
     *        set the attributes and call the associated init method
     *        if it should be a central and peripheral, start with the initialitation
     *        of the peripheral and wait till the connection is etablished
     *        after that, start the initialitation of the central
     * 
     * @param c true = central role
     * @param p true = peripheral role
    */
    void setDevice(bool c, bool p);

/*---------------------------------------------------------------------------
 * methods for peripheral and central role
 *--------------------------------------------------------------------------*/    

     /**
     * @brief callback function is called when connection is etablished
     * 
     * @param conn etablished connection structure
     * @param err error code, 0 if success
    */
    static void connected(struct bt_conn *conn, uint8_t err);

    /**
     * @brief callback function is called when connection gets disconnected
     * 
     * @param conn structure of the disconnected connection
     * @param reason reason code for the disconnection
    */
    static void disconnected(struct bt_conn *conn, uint8_t reason);

    /**
     * @brief LE connection parameter update request
     * 
     * @param conn the connection structure
     * @param param proposed connection parameters
     * @return true to accept the parameters
     * @return false to reject the parameter
     */
    static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param);

    /**
     * @brieftThe parameters for an LE connection have been updated
     * 
     * @param conn the connection structure
     * @param interval connection interval
     * @param latency connection latency
     * @param timeout connection supervision timeout
     */
    static void le_param_updated(struct bt_conn *conn, uint16_t interval,
				 uint16_t latency, uint16_t timeout);

/*--------------------------------------------------------------------------
 * methods for peripheral role
 *--------------------------------------------------------------------------*/

    /**
     * @brief initialize peripheral ble part
     * 
    */
    void initPeripheral();

    /**
     * @brief start with the advertising process
     * 
    */
    static void startAdvertising();

    /**
     * @brief send notification that button state has changed
     * 
     * @param button_state new button state
     * @param has_changed the button number
     */
    static void buttonChanged(uint32_t button_state, uint32_t has_changed);

    /**
     * @brief get button state
     * 
     * @return true button pressed
     * @return false button released
     */
    static bool app_button_cb();

    /**
     * @brief initialize button service
     * 
     * @return uint8_t error code, 0 if success
     */
    static uint8_t initButton(void);

    /**
     * @brief send notification with new led state
     * 
     * @param led_state true led on, false led off
     */
    static void app_led_cb(bool led_state);

/*---------------------------------------------------------------------------
 * methods for central role
 *--------------------------------------------------------------------------*/

    /**
     * @brief initialize central ble part
     * 
    */
    void initCentral();

    /**
     * @brief initialize all for starting the scan process
     * 
    */
    static void initScan();

    /**
     * @brief start the scanning process
     * 
    */
    static void startScan();

    /**
     * @brief start scanning after disconnect
     * 
     * @param type of sensors which has disconnected
     */
    static void reScan(uint8_t type);

    /**
     * @brief callback function, is called when a device is found
     *        used at the start befor scan with filters
     * 
     * @param addr address of the found device
     * @param rssi rssi value of the found device in db
     * @param type tyoe of the found device
     * @param ad data buffer
    */
    static void deviceFound(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                            struct net_buf_simple *ad);
    
    // add service callbacks
    /**
     * @brief callback function, is called when discovery of the service is completed
     * 
     * @param dm holds the address of the connected device, user context and discover parameter        
     * @param ctx context, not used in this case
    */
    static void discoveryCompletedCSC(struct bt_gatt_dm *dm, void *ctx);

    /**
     * @brief callback function, is called when service was not found
     * 
     * @param conn connection structure which does not include the service
     * @param ctx context, not used in this case
    */
    static void discovery_service_not_found(struct bt_conn *conn, void *ctx);

    /**
     * @brief callback function, is called when an error occurs while discovering the service
     * 
     * @param conn connection structure which the error occurs
     * @param err error code, 0 if success
     * @param ctx context, not used in this case
    */
    static void discovery_error_found(struct bt_conn *conn, int err, void *ctx);

    /**
     * @brief callback function, is called when discovery of the service is completed
     * 
     * @param dm holds the address of the connected device, user context and discover parameter
     * @param ctx context, not used in this case
     */
    static void discoveryCompletedHR(struct bt_gatt_dm *dm, void *ctx);

    /**
     * @brief start discovery process for the heart rate sensor
     * 
     */
    static void discoverHR();

    /**
     * @brief start discovery process for the CSC sensors
     * 
     */
    static void discoverCSC();

    /**
     * @brief callback function, is called when new data is received over ble
     * 
     * @param conn connection structure which sends the data
     * @param params subscribe parameter
     * @param data the received data
     * @param length the length of the received data
     * @return uint8_t value to continue
     */
    static uint8_t notify_HR(struct bt_conn *conn,
		struct bt_gatt_subscribe_params *params,
		const void *data, uint16_t length);

    /**
     * @brief callback function, is called when new data is received over ble
     * 
     * @param conn connection structure which sends the data
     * @param params subscribe parameter, not used in this case
     * @param data the received data
     * @param length the length of the received data
     * @return uint8_t value to continue
    */    
    static uint8_t onReceived(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length);

    /**
     * @brief callback function, is called when a device with the applicable filter is found
     * 
     * @param device_info information structure about the found device
     * @param filter_match information about the used filter
     * @param connectable information if the found device is connectable 
    */    
    static void scanFilterMatch(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable);

    /**
     * @brief callback function, is called when an error occurs while scanning
     * 
     * @param device_info information structure about the found device
    */                     
    static void scanConnectionError(struct bt_scan_device_info *device_info);

    /**
     * @brief callback function, is called when device found but not with the given filter
     * 
     * @param device_info info about the found device
     * @param connectable info if device is connectable
     */
    static void scanFilterNoMatch(struct bt_scan_device_info *device_info, bool connectable);

    /**
     * @brief compare two addresses
     * 
     * @param addr1 first address to compare
     * @param addr2 second address to compare
     * @return true if the addresses are equal
     * @return false if the addresses are not equal
    */
    static bool checkAddresses(char addr1[],char addr2[]);

private:    
    /*
     * private attributes 
     */
    static bool isCentral;
    static bool isPeripheral;
    static bool app_button_state;
    static bool subscriptionDone;
    static bool batterySubscriptionDone;
    static bool batterySubscriptionCSCDone; 
    static bool diameterSet;
    static bool once_sensor1;
	static bool once_sensor2;
    static bool once_sensor3;
    static bool reconnectedHeartRate;
    static uint8_t nbrAddresses;
    static uint8_t cntBatterySubscriptions;
    static uint8_t nbrConnectionsCentral;
    static uint8_t sensorInfos;

    static char sensor1[17]; 
    static char sensor2[17];
    static char sensor3[17];

    // array of central connections
    static struct bt_conn *centralConnections[MAX_CONNECTIONS_CENTRAL];

    // peripheral connection, there is just one -> with the android application
    static struct bt_conn *peripheralConn;

    // data object, containts all the received data with the calculate functions
    static dataCSC data;

    // array of subscribe parameters -> for every connection one parameter
    static struct bt_gatt_subscribe_params subscribe_params[MAX_CONNECTIONS_CENTRAL];

    // connection/disconnection callback structure
    struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
        .le_param_req = le_param_req,
        .le_param_updated = le_param_updated,
    };

    // led & button callback structure
    struct bt_lbs_cb lbs_callbacs = {
        .led_cb    = app_led_cb,
        .button_cb = app_button_cb,
    };

    // connection authorize callback structure
    struct bt_conn_auth_cb conn_auth_callbacks = {
    };
};
