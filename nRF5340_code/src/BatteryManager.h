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

#include <bluetooth/services/bas_client.h>

/*---------------------------------------------------------------------------
 * DEFINES
 *--------------------------------------------------------------------------*/ 
#define SPEED 0
#define CADENCE 1
#define HEARTRATE 2
#define DEFAULT 3

/**
 * @brief callback function, is called when discovery is completed
 * 
 * @param dm holds the address of the connected device, user context and discover parameter 
 * @param context context, not used in this case
 */
void discovery_completed_cb(struct bt_gatt_dm *dm, void *context);

/**
 * @brief callback function, is called when the battery service was not found
 * 
 * @param conn connection structure which does not include the service
 * @param context context, not used in this case
 */
void discovery_service_not_found_cb(struct bt_conn *conn, void *context);

/**
 * @brief callback function, is called when an error occurs while discovering the battery service
 * 
 * @param conn connection structure which the error occurs
 * @param err error code, 0 if success
 * @param context context, not used in this case
 */
void discovery_error_found_cb(struct bt_conn *conn, int err, void *context);

/**
 * @brief start discovering the battery service
 * 
 * @param conn the connection to search the battery service 
 */
uint8_t gatt_discover_battery_service(struct bt_conn *conn);

/**
 * @brief callback function, is called when user requests the battery level of speed sensor
 * 
 * @param bas the battery service client
 * @param battery_level the battery level (0-100%)
 * @param err the error code, 0 if success
 */
void read_battery_level_cb_speed(struct bt_bas_client *bas, uint8_t battery_level, int err);

/**
 * @brief callback function, is called when user requests the battery level of cadence sensor
 * 
 * @param bas the battery service client
 * @param battery_level the battery level (0-100%)
 * @param err the error code, 0 if success
 */
void read_battery_level_cb_cadence(struct bt_bas_client *bas, uint8_t battery_level, int err);

/**
 * @brief callback function, is called when user requests the battery level of heart rate sensor
 * 
 * @param bas the battery service client
 * @param battery_level the battery level (0-100%)
 * @param err the error code, 0 if success
 */
void read_battery_level_cb_heartRate(struct bt_bas_client *bas, uint8_t battery_level, int err);

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
 * @param sensorInfos info about which sensors are connected
 */
void initBatteryManager(uint8_t sensorInfos);

/**
 * @brief subscribe the battery service for speed sensor
 * 
 * @param dm holds the address of the connected device, user context and discover parameter 
 */
void subscribeBatterySpeed(struct bt_gatt_dm *dm);

/**
 * @brief subscribe the battery service for cadence sensor
 * 
 * @param dm holds the address of the connected device, user context and discover parameter  
 */
void subscribeBatteryCadence(struct bt_gatt_dm *dm);

/**
 * @brief subscribe the battery service for heart rate sensor
 * 
 * @param dm holds the address of the connected device, user context and discover parameter  
 */
void subscribeBatteryHeartRate(struct bt_gatt_dm *dm);

/**
 * @brief check if battery service client is busy
 * 
 * @return true if its free
 * @return false if its busy
 */
bool isFree();

/**
 * @brief ask heart rate sensor for the battery level
 * 
 * @param type which sensor ask for battery level
 */
void askForBatteryLevel(uint8_t type);

/**
 * @brief check if the battery level value is ready to read
 * 
 * @param type which sensor is checking ready attribute
 * @return true when battery level value is ready to read
 * @return false when battery level value is not ready to read
 */
bool isValueReady(uint8_t type);

/**
 * @brief reset the ready value
 * 
 * @param type which sensor wants to reset ready attribute
 */
void resetReadyValue(uint8_t type);