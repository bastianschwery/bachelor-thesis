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

bt_bas_client BatteryManager::bas;

BatteryManager::BatteryManager()
{
	
}

void BatteryManager::discovery_completed_cb(struct bt_gatt_dm *dm, void *context)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_bas_handles_assign(dm, &bas);
	if (err) {
		printk("Could not init BAS client object, error: %d\n", err);
	}

	if (bt_bas_notify_supported(&bas)) {
		err = bt_bas_subscribe_battery_level(&bas,
						     notify_battery_level_cb);
		if (err) {
			printk("Cannot subscribe to BAS value notification "
				"(err: %d)\n", err);
		}
	} else {
		err = bt_bas_start_per_read_battery_level(
			&bas, BAS_READ_VALUE_INTERVAL, notify_battery_level_cb);
		if (err) {
			printk("Could not start periodic read of BAS value\n");
		}
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

void BatteryManager::notify_battery_level_cb(struct bt_bas_client *bas,
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

	printk("Starting Bluetooth Central BAS example\n");

	bt_bas_client_init(&bas);
	
	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize buttons (err %d)\n", err);
		return;
	}
}

uint8_t BatteryManager::getBatteryLevel() 
{
    int err;

	printk("Reading BAS value:\n");
	err = bt_bas_read_battery_level(&bas, read_battery_level_cb);
	if (err)
    {
		printk("BAS read call error: %d\n", err);
	}

    return 8;
}

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

