/**
 * @file    BatteryManager.h
 * @author  Schwery Bastian (bastian98@gmx.ch)
 * @brief   Handles battery service -> gets new battery values
 * @version 0.1
 * @date    2021-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <inttypes.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/bas_client.h>
#include <dk_buttons_and_leds.h>

#include <settings/settings.h>

// defines
#define KEY_READVAL_MASK DK_BTN1_MSK

#define BAS_READ_VALUE_INTERVAL (10 * MSEC_PER_SEC)


/**
 * @brief Construct a new Battery Manager object
 * 
 */
//BatteryManager();

/**
 * @brief callback function, is called when new battery data received over ble
 * 
 * @param bas the battery service client 
 * @param battery_level the battery level (0-100%)
 */
static void notify_battery_level_cb_sensor1(struct bt_bas_client *bas, uint8_t battery_level);

/**
 * @brief callback function, is called when new battery data received over ble
 * 
 * @param bas the battery service client 
 * @param battery_level the battery level (0-100%)
 */
static void notify_battery_level_cb_sensor2(struct bt_bas_client *bas, uint8_t battery_level);

/**
 * @brief callback function, is called when new battery data received over ble
 * 
 * @param bas the battery service client 
 * @param battery_level the battery level (0-100%)
 */
static void notify_battery_level_cb_sensor3(struct bt_bas_client *bas, uint8_t battery_level);

/**
 * @brief callback function, is called when discovery is completed
 * 
 * @param dm holds the address of the connected device, user context and discover parameter 
 * @param context context, not used in this case
 */
static void discovery_completed_cb(struct bt_gatt_dm *dm, void *context);

/**
 * @brief callback function, is called when the battery service was not found
 * 
 * @param conn connection structure which does not include the service
 * @param context context, not used in this case
 */
static void discovery_service_not_found_cb(struct bt_conn *conn, void *context);

/**
 * @brief callback function, is called when an error occurs while discovering the battery service
 * 
 * @param conn connection structure which the error occurs
 * @param err error code, 0 if success
 * @param context context, not used in this case
 */
static void discovery_error_found_cb(struct bt_conn *conn, int err, void *context);

/**
 * @brief start discovering the battery service
 * 
 * @param conn the connection to search the battery service 
 */
uint8_t gatt_discover_battery_service(struct bt_conn *conn);

/**
 * @brief callback function, is called when user requests the battery level
 * 
 * @param bas the battery service client
 * @param battery_level the battery level (0-100%)
 * @param err the error code, 0 if success
 */
static void read_battery_level_cb(struct bt_bas_client *bas, uint8_t battery_level, int err);


static void read_battery_level_cb2(struct bt_bas_client *bas, uint8_t battery_level, int err);

static void read_battery_level_cb3(struct bt_bas_client *bas, uint8_t battery_level, int err);
/**
 * @brief get battery level when button 1 is pressed
 * 
 */
static void button_readval(void);

/**
 * @brief callback function, is called when a button is pressed on the board
 * 
 * @param button_state the state of the button, true = pressed - false = released
 * @param has_changed 
 */
static void button_handler(uint32_t button_state, uint32_t has_changed);

/**
 * @brief get the battery level 
 * 
 * @param nbrSensor from which sensor the battery level is requested
 * @return uint8_t the battery level (0-100%)
 */
uint8_t getBatteryLevel(uint8_t nbrSensor);

/**
 * @brief initialize the battery manager
 * 
 */
void initBatteryManager();


/*static struct bt_bas_client bas_sensor1;
static struct bt_bas_client bas_sensor2;
static struct bt_bas_client bas_sensor3;

static uint8_t batteryLevel_sensor1;
static uint8_t batteryLevel_sensor2;
static uint8_t batteryLevel_sensor3;
static uint8_t cntDevices;*/
