/**
 * @author  Schwery Bastian
 * @file    deviceManager.h    
 * @date    05/2021
 * 
 * @brief    This class manages the peripheral and central
 *           bluetooth connections and the characterstic
 *           subscriptions
 */

#include "dataCSC.h"
#include "dataService.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/addr.h>
#include <bluetooth/conn.h>
#include <string.h>

#include <bluetooth/uuid.h>
#include <bluetooth/hci.h>
#include <bluetooth/services/lbs.h>
#include <dk_buttons_and_leds.h>
#include <settings/settings.h>
#include <bluetooth/scan.h>

//#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/att.h>
#include <sys/byteorder.h>
//#include "../../xf/xf.h"

//using namespace std;
#define MAX_CONNECTIONS_CENTRAL 5
#define NBR_WANTED_CONNECTIONS 2

class deviceManager {
public:
/*---------------------------------------------------------------------------
 * general methods
 *--------------------------------------------------------------------------*/ 

    // constructor
    deviceManager();

    /**
     * @brief getter
     * @return 3 when central & peripheral role
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
     * @param c true = central role
     * @param p true = peripheral role
    */
    void setDevice(bool c, bool p);

/*---------------------------------------------------------------------------
 * methods for peripheral and central role
 *--------------------------------------------------------------------------*/    

     /**
     * @brief callback function is called when connection is etablished
     * @param conn etablished connection structure
     * @param err error code, 0 if success
    */
    static void connected(struct bt_conn *conn, uint8_t err);

    /**
     * @brief callback function is called when connection gets disconnected
     * @param conn structure of the disconnected connection
     * @param reason reason code for the disconnection
    */
    static void disconnected(struct bt_conn *conn, uint8_t reason);

/*--------------------------------------------------------------------------
 * methods for peripheral role
 *--------------------------------------------------------------------------*/

    /**
     * @brief initialize peripheral ble part
    */
    void initPeripheral();

    /**
     * @brief start with the advertising process
    */
    static void startAdvertising();

    static void buttonChanged(uint32_t button_state, uint32_t has_changed);
    static bool app_button_cb();
    static void app_led_cb(bool led_state);

/*---------------------------------------------------------------------------
 * methods for central role
 *--------------------------------------------------------------------------*/

    /**
     * @brief initialize central ble part
    */
    void initCentral();

    /**
     * @brief initialize all for starting the scan process
    */
    static void initScan();

    /**
     * @brief start the scanning process
    */
    static void startScan();

    /**
     * @brief callback function, is called when a device is found
     *        used to find the correct thingy
     * @param addr address of the found device
     * @param rssi rssi value of the found device in db
     * @param type tyoe of the found device
     * @param ad data buffer
    */
    static void deviceFound(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                            struct net_buf_simple *ad);
    
    // add service callbacks
    /**
     * @brief callback function, is called when discovery of the service 
     *        is completed
     * @param disc holds the address of the connected device, user context and 
     *             discover parameter
     * @param ctx context, not used in this case
    */
    static void discoveryCompleted(struct bt_gatt_dm *disc, void *ctx);

    /**
     * @brief callback function, is called when service was not found
     * @param conn connection structure which does not include the service
     * @param ctx context, not used in this case
    */
    static void discovery_service_not_found(struct bt_conn *conn, void *ctx);

    /**
     * @brief callback function, is called when an error occurs while discovering the service
     * @param conn connection structure which the error occurs
     * @param ctx context, not used in this case
    */
    static void discovery_error_found(struct bt_conn *conn, int err, void *ctx);

    /**
     * @brief callback function, is called when new data is received over ble
     * @param conn connection structure which sends the data
     * @param params subscribe parameter, not used in this case
     * @param data the received data
     * @param length the length of the received data
    */    
    static uint8_t onReceived(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length);

    /**
     * @brief callback function, is called when a device with the applicable filter is found
     * @param device_info information structure about the found device
     * @param filter_match information about the used filter
     * @param connectable information if the found device is connectable 
    */    
    static void scanFilterMatch(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable);

   /**
     * @brief callback function, is called when an error occurs while scanning
     * @param device_info information structure about the found device
    */                     
    static void scanConnectionError(struct bt_scan_device_info *device_info);

private:    
    /*
     * private attributes 
     */
    static bool isCentral;
    static bool isPeripheral;
    static bool connectedC;
    static bool connectedP;
    static bool app_button_state;
    static bool subscriptionDone;
    static bool diameterSet;
    static bool once_speed;
	static bool once_cadence;

    static int initButton(void);
    static uint8_t nbrConnectionsCentral;

    // array of central connections
    static struct bt_conn *centralConnections[MAX_CONNECTIONS_CENTRAL];

    // peripheral connection, there is just one -> with the android application
    static struct bt_conn *peripheralConn;

    // data object, containts all the received data with the calculate functions
    static dataCSC data;

    // connection/disconnection callback structure
    struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
    };

    // led & button callback structure
    struct bt_lbs_cb lbs_callbacs = {
        .led_cb    = app_led_cb,
        .button_cb = app_button_cb,
    };

    struct bt_conn_auth_cb conn_auth_callbacks = {
    };
};