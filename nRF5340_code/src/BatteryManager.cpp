/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic Battery Service Client sample
 */
#include "BatteryManager.h"
#include <bluetooth/services/bas_client.h>

//static struct bt_bas_client bas;

bt_bas_client BatteryManager::bas_sensor1;
bt_bas_client BatteryManager::bas_sensor2;
bt_bas_client BatteryManager::bas_sensor3;
uint8_t BatteryManager::cntDevices = 0;
uint8_t BatteryManager::batteryLevel_sensor1 = 0;
uint8_t BatteryManager::batteryLevel_sensor2 = 0;
uint8_t BatteryManager::batteryLevel_sensor3 = 0;

BatteryManager::BatteryManager()
{
	
}

void BatteryManager::discovery_completed_cb(struct bt_gatt_dm *dm, void *context)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	switch (cntDevices)
	{
	case 1:
		err = bt_bas_handles_assign(dm, &bas_sensor1);
		if (err) 
		{
			printk("Could not init BAS client object from sensor 1, error: %d\n", err);
		}

		if (bt_bas_notify_supported(&bas_sensor1))
		{
			err = bt_bas_subscribe_battery_level(&bas_sensor1, notify_battery_level_cb_sensor1);
			if (err) 
			{
				printk("Cannot subscribe to BAS value notification from sensor 1, (err: %d)\n", err);
			}
		} else 
		{
			err = bt_bas_start_per_read_battery_level(&bas_sensor1, BAS_READ_VALUE_INTERVAL, notify_battery_level_cb_sensor1);
			if (err) 
			{
				printk("Could not start periodic read of BAS value for sensor 1\n");
			}
		}
		break;
	case 2:
		err = bt_bas_handles_assign(dm, &bas_sensor2);
		if (err) 
		{
			printk("Could not init BAS client object from sensor 2, error: %d\n", err);
		}

		if (bt_bas_notify_supported(&bas_sensor2))
		{
			err = bt_bas_subscribe_battery_level(&bas_sensor2, notify_battery_level_cb_sensor2);
			if (err) 
			{
				printk("Cannot subscribe to BAS value notification from sensor 2, (err: %d)\n", err);
			}
		} else 
		{
			err = bt_bas_start_per_read_battery_level(&bas_sensor2, BAS_READ_VALUE_INTERVAL, notify_battery_level_cb_sensor2);
			if (err) 
			{
				printk("Could not start periodic read of BAS value for sensor 2\n");
			}
		}	
		break;
	case 3:
		err = bt_bas_handles_assign(dm, &bas_sensor3);
		if (err) 
		{
			printk("Could not init BAS client object from sensor 3, error: %d\n", err);
		}

		if (bt_bas_notify_supported(&bas_sensor3))
		{
			err = bt_bas_subscribe_battery_level(&bas_sensor3, notify_battery_level_cb_sensor3);
			if (err) 
			{
				printk("Cannot subscribe to BAS value notification from sensor 3, (err: %d)\n", err);
			}
		} else 
		{
			err = bt_bas_start_per_read_battery_level(&bas_sensor3, BAS_READ_VALUE_INTERVAL, notify_battery_level_cb_sensor3);
			if (err) 
			{
				printk("Could not start periodic read of BAS value for sensor 3\n");
			}
		}	
		break;
	default:
		break;
	}

	

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
}

void BatteryManager::discovery_service_not_found_cb(struct bt_conn *conn,
					   void *context)
{
	printk("The service could not be found during the discovery\n");
}

void BatteryManager::discovery_error_found_cb(struct bt_conn *conn,
				     int err,
				     void *context)
{
	printk("The discovery procedure failed with %d\n", err);
}

void BatteryManager::gatt_discover(struct bt_conn *conn)
{
	int err;

    static struct bt_gatt_dm_cb discovery_cb = {
        .completed = discovery_completed_cb,
        .service_not_found = discovery_service_not_found_cb,
        .error_found = discovery_error_found_cb,
    };

	err = bt_gatt_dm_start(conn, BT_UUID_BAS, &discovery_cb, NULL);
	if (err) {
		printk("Could not start the discovery procedure, error "
		       "code: %d\n", err);
	}
}

void BatteryManager::notify_battery_level_cb_sensor1(struct bt_bas_client *bas,
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

	batteryLevel_sensor1 = battery_level;
}

void BatteryManager::notify_battery_level_cb_sensor2(struct bt_bas_client *bas,
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

	batteryLevel_sensor2 = battery_level;
}

void BatteryManager::notify_battery_level_cb_sensor3(struct bt_bas_client *bas,
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

	batteryLevel_sensor3 = battery_level;
}

void BatteryManager::read_battery_level_cb(struct bt_bas_client *bas,
				  uint8_t battery_level,
				  int err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	//bt_addr_le_to_str(bt_conn_get_dst(bt_bas_conn(bas)),
			//  addr, sizeof(addr));
	if (err) {
		printk("[%s] Battery read ERROR: %d\n", addr, err);
		return;
	}

	printk("[%s] Battery read: %"PRIu8"%%\n", addr, battery_level);
    //batteryLevel = battery_level;
}

void BatteryManager::button_readval(void)
{
	int err;

	printk("Reading BAS value:\n");
	//err = bt_bas_read_battery_level(&bas, read_battery_level_cb);
	if (err) {
		printk("BAS read call error: %d\n", err);
	}
}


void BatteryManager::button_handler(uint32_t button_state, uint32_t has_changed)
{
	uint32_t button = button_state & has_changed;

	if (button & KEY_READVAL_MASK) {
		button_readval();
	}
}

void BatteryManager::initBatteryManager(void)
{
	int err;
	cntDevices++;
	printk("Starting Bluetooth Central BAS example\n");

	switch (cntDevices)
	{
	case 1:
		bt_bas_client_init(&bas_sensor1);
		break;
	case 2:
		bt_bas_client_init(&bas_sensor2);
		break;
	case 3:
		bt_bas_client_init(&bas_sensor3);
		break;
	default:
		break;
	}
	
	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize buttons (err %d)\n", err);
		return;
	}
}

uint8_t BatteryManager::getBatteryLevel(uint8_t nbrSensor) 
{
    int err;

	printk("Reading BAS value:\n");

	switch (nbrSensor)
	{
	case 1:
		err = bt_bas_read_battery_level(&bas_sensor1, read_battery_level_cb);
		break;
	case 2:
		err = bt_bas_read_battery_level(&bas_sensor2, read_battery_level_cb);
		break;
	case 3:
		err = bt_bas_read_battery_level(&bas_sensor3, read_battery_level_cb);
		break;
	default:
		break;
	}

	if (err)
    {
		printk("BAS read call error: %d\n", err);
	}

    return err;
}
