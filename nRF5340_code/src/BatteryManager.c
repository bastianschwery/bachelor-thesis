#include "BatteryManager.h"
#include <bluetooth/services/bas_client.h>

// global static attributs
static struct bt_bas_client bas_speed;
static struct bt_bas_client bas_cadence;
static struct bt_bas_client bas_heartRate;

static uint8_t batteryLevel_speed;
static uint8_t batteryLevel_cadence;
static uint8_t batteryLevel_heartRate;
static uint8_t cntDevices = 0;
static uint8_t infoSensors = 0;
static bool free = true;

void discovery_completed_cb(struct bt_gatt_dm *dm, void *context)
{
	int err;
	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	switch (infoSensors)
	{
	case 1:
		subscribeBatterySpeed(dm);
		break;
	case 2:
		subscribeBatteryCadence(dm);
		break;
	case 3:
		if (cntDevices == 1)
		{
			subscribeBatterySpeed(dm);
		}
		else
		{
			subscribeBatteryCadence(dm);
		}
		break;
	case 4:
		if (cntDevices == 1)
		{
			subscribeBatterySpeed(dm);
		}
		else if (cntDevices == 2)
		{
			subscribeBatteryCadence(dm);
		}		
		else 
		{
			subscribeBatteryHeartRate(dm);
		}
		break;
	case 5:
		if (cntDevices == 1)
		{
			subscribeBatterySpeed(dm);
		}
		else
		{
			subscribeBatteryHeartRate(dm);
		}
		break;
	case 6:
		if (cntDevices == 1)
		{
			subscribeBatteryCadence(dm);
		}
		else
		{
			subscribeBatteryHeartRate(dm);
		}		
		break;		
	case 7:
			subscribeBatteryHeartRate(dm);
		break;							
	default:
		break;
	}
}

void discovery_service_not_found_cb(struct bt_conn *conn, void *context)
{
	printk("The service could not be found during the discovery\n");
}

void discovery_error_found_cb(struct bt_conn *conn,
				     int err,
				     void *context)
{
	printk("The discovery procedure failed with %d\n", err);
}

uint8_t gatt_discover_battery_service(struct bt_conn *conn)
{
	int err;

	static uint8_t cnt = 0;
	cnt++;
	
    static struct bt_gatt_dm_cb discovery_cb = {
        .completed = discovery_completed_cb,
        .service_not_found = discovery_service_not_found_cb,
        .error_found = discovery_error_found_cb,
    };

	err = bt_gatt_dm_start(conn, BT_UUID_BAS, &discovery_cb, NULL);
	if (err) {
		printk("Could not start the discovery procedure, error "
		       "code: %d\n", err);
		cnt--;
		cntDevices--;
	}
	return err;
}

void notify_battery_level_cb_speed(struct bt_bas_client *bas,
				    uint8_t battery_level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (battery_level == BT_BAS_VAL_INVALID) {
		printk("[%s] Battery notification aborted\n", addr);
	} else {
		printk("[%s] Battery notification: %"PRIu8"%%\n",
		       addr, battery_level);
	}

	batteryLevel_speed = battery_level;
}

void notify_battery_level_cb_cadence(struct bt_bas_client *bas,
				    uint8_t battery_level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (battery_level == BT_BAS_VAL_INVALID) {
		printk("[%s] Battery notification aborted\n", addr);
	} else {
		printk("[%s] Battery notification: %"PRIu8"%%\n",
		       addr, battery_level);
	}

	batteryLevel_cadence = battery_level;
}

void notify_battery_level_cb_heartRate(struct bt_bas_client *bas,
				    uint8_t battery_level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (battery_level == BT_BAS_VAL_INVALID) {
		printk("[%s] Battery notification aborted\n", addr);
	} else {
		printk("[%s] Battery notification: %"PRIu8"%%\n",
		       addr, battery_level);
	}

	batteryLevel_heartRate = battery_level;
}

void read_battery_level_cb_speed(struct bt_bas_client *bas,
				  uint8_t battery_level,
				  int err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (err) {
		printk("[%s] Battery read ERROR: %d\n", addr, err);
		return;
	}

	printk("[%s] Battery read: %"PRIu8"%%\n", addr, battery_level);
    batteryLevel_speed = battery_level;
}

void read_battery_level_cb_cadence(struct bt_bas_client *bas,
				  uint8_t battery_level,
				  int err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (err) {
		printk("[%s] Battery read ERROR: %d\n", addr, err);
		return;
	}

	printk("[%s] Battery read: %"PRIu8"%%\n", addr, battery_level);
    batteryLevel_cadence = battery_level;
}

void read_battery_level_cb_heartRate(struct bt_bas_client *bas,
				  uint8_t battery_level,
				  int err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (err) {
		printk("[%s] Battery read ERROR: %d\n", addr, err);
		return;
	}

	printk("[%s] Battery read: %"PRIu8"%%\n", addr, battery_level);
    batteryLevel_heartRate = battery_level;
}

void initBatteryManager(uint8_t sensorInfos)
{
	free = false;
	infoSensors = sensorInfos;
	int err;
	cntDevices++;
	printk("Initialize battery manager: # %d\n", cntDevices);

	switch (sensorInfos)
	{
	case 1:
		bt_bas_client_init(&bas_speed);
		break;
	case 2:
		bt_bas_client_init(&bas_cadence);
		break;
	case 3:
		if (cntDevices == 1)
		{
			bt_bas_client_init(&bas_speed);
		}
		else
		{
			bt_bas_client_init(&bas_cadence);
		}
		break;
	case 4:
		if (cntDevices == 1)
		{
			bt_bas_client_init(&bas_speed);
		}
		else if (cntDevices == 2)
		{
			bt_bas_client_init(&bas_cadence);
		}	
		else
		{
			bt_bas_client_init(&bas_heartRate);
		}
		break;
	case 5:
		if (cntDevices == 1)
		{
			bt_bas_client_init(&bas_speed);
		}
		else
		{
			bt_bas_client_init(&bas_heartRate);
		}
		break;
	case 6:
		if (cntDevices == 1)
		{
			bt_bas_client_init(&bas_cadence);
		}
		else
		{
			bt_bas_client_init(&bas_heartRate);
		}	
		break;
	case 7:
		bt_bas_client_init(&bas_heartRate);
		break;				
	default:
		break;
	}
}

uint8_t getBatteryLevel(uint8_t nbrSensor) 
{
    int err = 0;
	uint8_t level = 5;
	switch (nbrSensor)
	{
	case 1:		
		//bt_bas_read_battery_level(&bas_speed, read_battery_level_cb_speed);
		return batteryLevel_speed;
		break;
	case 2:
		return batteryLevel_cadence;
		break;
	case 3:
		//printk("Level: %d\n",level);
		return batteryLevel_heartRate;
		break;
	default:
		break;
	}

    return err;
}

void subscribeBatterySpeed(struct bt_gatt_dm *dm) 
{
	uint8_t err = 0;

	err = bt_bas_handles_assign(dm, &bas_speed);
	if (err) 
	{
		printk("Could not init BAS client object from speed sensor, error: %d\n", err);
	}

	if (bt_bas_notify_supported(&bas_speed))
	{
		err = bt_bas_subscribe_battery_level(&bas_speed, notify_battery_level_cb_speed);
		if (err) 
		{
			printk("Cannot subscribe to BAS value notification from speed sensor, (err: %d)\n", err);
		}
	} else 
	{
		err = bt_bas_start_per_read_battery_level(&bas_speed, BAS_READ_VALUE_INTERVAL, notify_battery_level_cb_speed);
		if (err) 
		{
			printk("Could not start periodic read of BAS value for speed sensor\n");
		}
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
	free = true;
}

void subscribeBatteryCadence(struct bt_gatt_dm *dm) 
{
	uint8_t err = 0;

	err = bt_bas_handles_assign(dm, &bas_cadence);
	if (err) 
	{
		printk("Could not init BAS client object from cadence sensor, error: %d\n", err);
	}

	if (bt_bas_notify_supported(&bas_cadence))
	{
		err = bt_bas_subscribe_battery_level(&bas_cadence, notify_battery_level_cb_cadence);
		if (err) 
		{
			printk("Cannot subscribe to BAS value notification from cadence sensor, (err: %d)\n", err);
		}
	} else 
	{
		err = bt_bas_start_per_read_battery_level(&bas_cadence, BAS_READ_VALUE_INTERVAL, notify_battery_level_cb_cadence);
		if (err) 
		{
			printk("Could not start periodic read of BAS value for cadence sensor\n");
		}
	}	

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
	free = true;
}

void subscribeBatteryHeartRate(struct bt_gatt_dm *dm) 
{
	uint8_t err = 0;

	err = bt_bas_handles_assign(dm, &bas_heartRate);
	if (err) 
	{
		printk("Could not init BAS client object from heart rate sensor, error: %d\n", err);
	}

	if (bt_bas_notify_supported(&bas_heartRate))
	{
		err = bt_bas_subscribe_battery_level(&bas_heartRate, notify_battery_level_cb_heartRate);
		if (err) 
		{
			printk("Cannot subscribe to BAS value notification from heart rate sensor, (err: %d)\n", err);
		}
	} else 
	{
		err = bt_bas_start_per_read_battery_level(&bas_heartRate, BAS_READ_VALUE_INTERVAL, notify_battery_level_cb_heartRate);
		if (err) 
		{
			printk("Could not start periodic read of BAS value for heart rate sensor\n");
		}
	}	

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}	
	free = true;
}

bool isFree()
{
	return free;
}

void askForBatteryLevelHeartRate()
{
	bt_bas_read_battery_level(&bas_heartRate, read_battery_level_cb_heartRate);
}