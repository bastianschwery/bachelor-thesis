#include "BatteryManager.h"


/*---------------------------------------------------------------------------
 * GLOBAL VARIABLES
 *--------------------------------------------------------------------------*/ 
static struct bt_bas_client clients[3];
static uint8_t batteryLevels[4];

static uint8_t cntDevices = 0;
static uint8_t infoSensors = 0;
static bool free = true;
static bool readyValues[3];
static bool service_found = true;

void discovery_completed_cb(struct bt_gatt_dm *dm, void *context)
{
	printk("The discovery procedure succeeded\n");
	service_found = true;
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
	service_found = false;
	cntDevices--;
	free = true;
}

void discovery_error_found_cb(struct bt_conn *conn,
				     int err,
				     void *context)
{
	printk("The discovery procedure failed with %d\n", err);
	cntDevices--;
	free = true;
}

uint8_t gatt_discover_battery_service(struct bt_conn *conn)
{
	uint8_t err;
	free = false;
	static uint8_t cnt = 0;
	cnt++;
	
    static struct bt_gatt_dm_cb discovery_cb = {
        .completed = discovery_completed_cb,
        .service_not_found = discovery_service_not_found_cb,
        .error_found = discovery_error_found_cb,
    };

	err = bt_gatt_dm_start(conn, BT_UUID_BAS, &discovery_cb, NULL);
	if (err) 
	{
		printk("Could not start the discovery procedure, error "
		       "code: %d\n", err);
		cnt--;
		cntDevices--;
	}
	return err;
}

void read_battery_level_cb_speed(struct bt_bas_client *bas,
				  uint8_t battery_level,
				  int err)
{
	readyValues[SPEED] = true;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (err) 
	{
		printk("[%s] Battery read ERROR: %d\n", addr, err);
		return;
	}

	printk("[%s] Battery read: %"PRIu8"%%\n", addr, battery_level);
	batteryLevels[SPEED] = battery_level;
}

void read_battery_level_cb_cadence(struct bt_bas_client *bas,
				  uint8_t battery_level,
				  int err)
{
	readyValues[CADENCE] = true;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (err) 
	{
		printk("[%s] Battery read ERROR: %d\n", addr, err);
		return;
	}

	printk("[%s] Battery read: %"PRIu8"%%\n", addr, battery_level);
	batteryLevels[CADENCE] = battery_level;
}

void read_battery_level_cb_heartRate(struct bt_bas_client *bas,
				  uint8_t battery_level,
				  int err)
{
	readyValues[HEARTRATE] = true;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			  addr, sizeof(addr));
	if (err) 
	{
		printk("[%s] Battery read ERROR: %d\n", addr, err);
		return;
	}

	printk("[%s] Battery read: %"PRIu8"%%\n", addr, battery_level);
	batteryLevels[HEARTRATE] = battery_level;
}

void initBatteryManager(uint8_t sensorInfos)
{
	for (uint8_t i = 0; i <= 2; i++)
	{
		readyValues[i] = false;
		batteryLevels[i] = 0;
	}
	
	free = false;
	infoSensors = sensorInfos;
	cntDevices++;
	printk("Initialize battery manager: # %d\n", cntDevices);

	switch (sensorInfos)
	{
	case 1:
		bt_bas_client_init(&clients[SPEED]);
		break;
	case 2:
		bt_bas_client_init(&clients[CADENCE]);
		break;
	case 3:
		if (cntDevices == 1)
		{
			bt_bas_client_init(&clients[SPEED]);
		}
		else
		{
			bt_bas_client_init(&clients[CADENCE]);
		}
		break;
	case 4:
		if (cntDevices == 1)
		{
			bt_bas_client_init(&clients[SPEED]);
		}
		else if (cntDevices == 2)
		{
			bt_bas_client_init(&clients[CADENCE]);
		}	
		else
		{
			bt_bas_client_init(&clients[HEARTRATE]);
		}
		break;
	case 5:
		if (cntDevices == 1)
		{
			bt_bas_client_init(&clients[SPEED]);
		}
		else
		{
			bt_bas_client_init(&clients[HEARTRATE]);
		}
		break;
	case 6:
		if (cntDevices == 1)
		{
			bt_bas_client_init(&clients[CADENCE]);
		}
		else
		{
			bt_bas_client_init(&clients[HEARTRATE]);
		}	
		break;
	case 7:
		bt_bas_client_init(&clients[HEARTRATE]);
		break;				
	default:
		break;
	}
}

uint8_t getBatteryLevel(uint8_t nbrSensor) 
{
    uint8_t defaultValue = 0;
	switch (nbrSensor)
	{
	case 1:
		return batteryLevels[SPEED];	
		break;
	case 2:
		return batteryLevels[CADENCE];
		break;
	case 3:
		return batteryLevels[HEARTRATE];
		break;
	default:
		batteryLevels[DEFAULT] = 0;
		return batteryLevels[DEFAULT];
		break;
	}

    return defaultValue;
}

void subscribeBatterySpeed(struct bt_gatt_dm *dm) 
{
	uint8_t err = 0;

	err = bt_bas_handles_assign(dm, &clients[SPEED]);
	if (err) 
	{
		printk("Could not init BAS client object from speed sensor, error: %d\n", err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) 
	{
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
	free = true;
}

void subscribeBatteryCadence(struct bt_gatt_dm *dm) 
{
	uint8_t err = 0;

	err = bt_bas_handles_assign(dm, &clients[CADENCE]);
	if (err) 
	{
		printk("Could not init BAS client object from cadence sensor, error: %d\n", err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) 
	{
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}

	free = true;
}

void subscribeBatteryHeartRate(struct bt_gatt_dm *dm) 
{
	uint8_t err = 0;

	err = bt_bas_handles_assign(dm, &clients[HEARTRATE]);
	if (err) 
	{
		printk("Could not init BAS client object from heart rate sensor, error: %d\n", err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) 
	{
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}	
	
	free = true;
}

bool isFree()
{
	return free;
}

void askForBatteryLevel(uint8_t type)
{
	switch (type)
	{
	case 1:
		readyValues[SPEED] = false;
		bt_bas_read_battery_level(&clients[SPEED], read_battery_level_cb_speed);
		break;
	case 2: 
		readyValues[CADENCE] = false;
		bt_bas_read_battery_level(&clients[CADENCE], read_battery_level_cb_cadence);
		break;
	case 3:
		readyValues[HEARTRATE] = false;
		bt_bas_read_battery_level(&clients[HEARTRATE], read_battery_level_cb_heartRate);
		break;
	default:
		break;
	}
}

bool isValueReady(uint8_t type)
{
	switch (type)
	{
	case 1:
		return readyValues[SPEED];
		break;
	case 2:
		return readyValues[CADENCE];
		break;
	case 3:
		return readyValues[HEARTRATE];
		break;			
	default:
		return false;
		break;
	}	
}

void resetReadyValue(uint8_t type)
{
	switch (type)
	{
	case 1:
		readyValues[SPEED] = false;
		break;
	case 2:
		readyValues[CADENCE] = false;
		break;
	case 3:
		readyValues[HEARTRATE] = false;
		break;			
	default:
		break;
	}		
}

bool serviceFound()
{
	return service_found;
}