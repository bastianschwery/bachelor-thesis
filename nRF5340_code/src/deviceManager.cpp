#include "DeviceManager.h"

// data service definition
BT_GATT_SERVICE_DEFINE(csc_srv,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CSC),
	BT_GATT_CHARACTERISTIC(BT_UUID_CSC_MEASUREMENT, BT_GATT_CHRC_NOTIFY,
			       0x00, NULL, NULL, NULL),			   
);

// initialize static attributes
bool DeviceManager::isCentral = false;
bool DeviceManager::isPeripheral = false;
bool DeviceManager::app_button_state = false;
bool DeviceManager::subscriptionDone = false;
bool DeviceManager::diameterSet = false;
bool DeviceManager::once_sensor1 = true;
bool DeviceManager::once_sensor2 = true;
bool DeviceManager::once_sensor3 = true;
bool DeviceManager::batterySubscriptionDone = false;
bool DeviceManager::serviceNotFound = false;
bool DeviceManager::reconnectedHeartRate = false;
bool DeviceManager::peripheralDisconnected = false;
bool DeviceManager::connectedPeripheral = false;
bool DeviceManager::cscDisconnected = false;
bool DeviceManager::hrDisconnected = false;
bool DeviceManager::disconnectOnce = true;
uint8_t DeviceManager::nbrAddresses = 0;
uint8_t DeviceManager::nbrConnectionsCentral = 0;
uint8_t DeviceManager::sensorInfos = 0;
uint8_t DeviceManager::cntBatterySubscriptions = 0;
uint8_t DeviceManager::cntFirstHR = 0;
char DeviceManager::sensor1[];
char DeviceManager::sensor2[];
char DeviceManager::sensor3[];

const bt_data DeviceManager::sd[] = {BT_DATA_BYTES(BT_DATA_UUID128_ALL, DATA_SERVICE_UUID),};
const bt_data DeviceManager::ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

bt_conn* DeviceManager::peripheralConn;
bt_conn* DeviceManager::centralConnections[];
bt_gatt_subscribe_params DeviceManager::subscribe_params[];
Data DeviceManager::data;

// define discovery callback for the CSC sensors
static struct bt_gatt_dm_cb discovery_cb_CSC = 
{
	.completed = DeviceManager::discoveryCompletedCSC,
	.service_not_found = DeviceManager::discovery_service_not_found,
	.error_found = DeviceManager::discovery_error_found,
};

// define discovery callback for the heart rate sensor
static struct bt_gatt_dm_cb discovery_cb_HR = 
{
	.completed = DeviceManager::discoveryCompletedHR,
	.service_not_found = DeviceManager::discovery_service_not_found,
	.error_found = DeviceManager::discovery_error_found,
};

/*-----------------------------------------------------------------------------------------------------
 * GENERAL METHODS
 *---------------------------------------------------------------------------------------------------*/

DeviceManager::DeviceManager()
{
	for (uint8_t i = 0; i <= MAX_CONNECTIONS_CENTRAL-1; i++)
	{
		centralConnections[i] = nullptr;
	}
}

uint8_t DeviceManager::getDevice()
{
    if (isCentral && isPeripheral)
	{
        return 3;
    } 
	else if (!isCentral && isPeripheral)
	{
        return 2;
    } 
	else if (isCentral && !isPeripheral)
	{
        return 1;
    } 
	else 
	{
        return 0;
    }
}

void DeviceManager::setDevice(bool c, bool p)
{
    isPeripheral = p;
    isCentral = c;  

	if (isCentral == true && isPeripheral == true)
	{
		initPeripheral();
	}
	else if (isCentral == true && isPeripheral == false)
	{
		initCentral();
	}
	else if (isCentral == false && isPeripheral == true)
	{
		initPeripheral();
	}
}

void DeviceManager::app_led_cb(bool led_state)
{
	// set led on board to led_state
    dk_set_led(USER_LED,led_state);
}

bool DeviceManager::app_button_cb(void)
{
    return app_button_state;
}

void DeviceManager::buttonChanged(uint32_t button_state, uint32_t has_changed)
{
	// button not used in this project -> can be used for other projects
   	if (has_changed & USER_BUTTON) {
		bt_lbs_send_button_state(button_state);
		app_button_state = button_state ? true : false;
	} 
}

uint8_t DeviceManager::initButton()
{
    uint8_t err;
	
	// initialize function buttonChanged as callback when state of button changes
    err = dk_buttons_init(buttonChanged);
	if (err) 
	{
		printk("Cannot init buttons (err: %d)\n", err);
	}

    return err;
}

/*-----------------------------------------------------------------------------------------------------
 * PERIPHERAL ROLE
 *---------------------------------------------------------------------------------------------------*/

void DeviceManager::initPeripheral()
{
    uint8_t err;
    if(getDevice() == 3 || getDevice() == 2)
	{
		// initialize leds
        err = dk_leds_init();
        if (err) 
		{
            printk("LEDs init failed (err %d)\n", err);
            return;
        }

		// initialize buttons
        err = initButton();
        if (err) 
		{
            printk("Button init failed (err %d)\n", err);
            return;
        }

		// enable bluetooth
        err = bt_enable(NULL);
        if (err) 
		{
            printk("Bluetooth init failed (err %d)\n", err);
            return;
        }

        printk("Bluetooth initialized\n");

		// register callback functions
		bt_conn_cb_register(&conn_callbacks);

		// config settings
        if (IS_ENABLED(CONFIG_SETTINGS)) 
		{
            settings_load();
        }

		// initialize Led Button Service
        err = bt_lbs_init(&lbs_callbacs);
        if (err) 
		{
            printk("Failed to init LBS (err:%d)\n", err);
            return;
        }

		// initialize data service
		err = data_service_init();
		if (err) 
		{
			printk("Failed to init LBS (err:%d)\n", err);
			return;
		}

		startAdvertising();
    }
}

void DeviceManager::startAdvertising() 
{
	uint8_t err;
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			sd, ARRAY_SIZE(sd));
	if (err) 
	{
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
	printk("Waiting for connection with application...\n");
}

/*-----------------------------------------------------------------------------------------------------
 * CENTRAL ROLE
 *---------------------------------------------------------------------------------------------------*/
 
void DeviceManager::initCentral()
{
	printk("Init Central\n");
	if (getDevice() == 1 || getDevice() == 3)
	{
	    uint8_t err;
		// when device = 3, initPeripheral is already called, most inits are already done
		if (getDevice() != 3)
		{
			// enable bluetooth
			err = bt_enable(nullptr);
			if (err)
			{
				printk("Bluetooth init failed (err %d)\n", err);
				return;
			}
			printk("Bluetooth ready\n");

			// initialize leds
			err = dk_leds_init();
			if (err) 
			{
				printk("LEDs init failed (err %d)\n", err);
				return;
			}

			// initialize buttons
			err = initButton();
			if (err) 
			{
				printk("Button init failed (err %d)\n", err);
				return;
			}

			// initialize Led Button Service
			err = bt_lbs_init(&lbs_callbacs);
			if (err) 
			{
				printk("Failed to init LBS (err:%d)\n", err);
				return;
			}

			// config settings
			if (IS_ENABLED(CONFIG_SETTINGS)) 
			{
				settings_load();
				printk("Settings loaded\n");
			}

			bt_conn_cb_register(&conn_callbacks);
		}

		initScan();
		startScan();	
	}
}

void DeviceManager::initScan() 
{
	uint8_t err;
	static bool once = true;
	sensorInfos = getSensorInfos();
	
	// scan parameter
	struct bt_le_scan_param scanParam = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
        .timeout = 0
    };	

	// scan init parameter
	struct bt_scan_init_param scanInit = {
		.scan_param = &scanParam,
		.connect_if_match = 0,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT,
	};

	if (sensorInfos != 0)
	{
		if (once)
		{
			// initialize scanning module of nordic
			once = false;
			BT_SCAN_CB_INIT(scan_cb, scanFilterMatch, NULL, scanConnectionError, NULL);
			bt_le_scan_stop();
			bt_scan_init(&scanInit);
			bt_scan_cb_register(&scan_cb);			
		}
		
		bt_scan_filter_remove_all();
		switch (sensorInfos)
		{
			case 1:
				// search just 1 speed sensor
				// add CSC UUID filter
				err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
				if (err) 
				{
					printk("Scanning filters cannot be set\n");
					return;
				}
				break;
			case 2:
				// search just 1 cadence sensor
				// add CSC UUID filter
				err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
				if (err) 
				{
					printk("Scanning filters cannot be set\n");
					return;
				}		
				break;
			case 3:
				// search just for CSC sensors
				// add CSC UUID filter
				err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
				if (err) 
				{
					printk("Scanning filters cannot be set\n");
					return;
				}
				break;
			case 4:
				// first search 2 CSC sensors, after a heart rate sensor
				if (nbrConnectionsCentral == 0 || nbrConnectionsCentral == 1)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
					if (err) 
					{
						printk("Scanning filters cannot be set\n");
						return;
					}
				}
				else if (nbrConnectionsCentral == 2)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
					if (err) 
					{
						printk("Scanning filters cannot be set\n");
						return;
					}
				}
				break;
			case 5:
				// first search for a speed sensor, after a heart rate sensor
				if (nbrConnectionsCentral == 0)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
					if (err) 
					{
						printk("Scanning filters cannot be set\n");
						return;
					}			
				}
				else if (nbrConnectionsCentral == 1)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
					if (err) 
					{
						printk("Scanning filters cannot be set\n");
						return;
					}
				}
				break;
			case 6:
				// first search for a cadence sensor, after a heart rate sensor
				if (nbrConnectionsCentral == 0)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
					if (err) 
					{
						printk("Scanning filters cannot be set\n");
						return;
					}			
				}
				else if (nbrConnectionsCentral == 1)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
					if (err) 
					{
						printk("Scanning filters cannot be set\n");
						return;
					}
				}
				break;			
			case 7:
				// just a heart rate sensor to connect
				err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
				if (err) 
				{
					printk("Scanning filters cannot be set\n");
					return;
				}	
				break;	
			default:
				break;
		}
		
		// enable filters
		err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
		if (err) 
		{
			printk("Filters cannot be turned on\n");
		}

		startScan();
	} 
	else 
	{
		bt_le_scan_start(&scanParam, deviceFound);
	}
}

void DeviceManager::startScan()
{
    uint8_t err;
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err)
	{
		printk("Scanning failed to start, err %d\n", err);
	}
	printk("Scanning...\n");
}

void DeviceManager::reScan(uint8_t type)
{
	uint8_t err = 0;

	bt_scan_filter_remove_all();	
	// start scan with type of disconnected sensor
	switch (type)
	{
	case 1:
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
		if (err) 
		{
			printk("Scanning filters cannot be set\n");
		}
		break;
	case 2:
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
		if (err) 
		{
			printk("Scanning filters cannot be set\n");
		}
		break;
	case 3:
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
		if (err) 
		{
			printk("Scanning filters cannot be set\n");
		}	
		break;		
	default:
		break;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) 
	{
		printk("Filters cannot be turned on\n");
	}
	startScan();
}

void DeviceManager::scanFilterMatch(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable) {

	static bool ready = false;
	nbrAddresses = getNbrOfAddresses();
	
	if (nbrAddresses != 0)
	{
		ready = true;
		// get addresses from data service
		switch (nbrAddresses)
		{
		case 1:
			getAddress(sensor1,1);
			break;
		case 2:
			getAddress(sensor1,1);
			getAddress(sensor2,2);
			break;
		case 3:
			getAddress(sensor1,1);
			getAddress(sensor2,2);
			getAddress(sensor3,3);
			break;
		default:
			break;
		}	
	}
	else 
	{
		initScan();
		ready = false;
	}
	
    char addr[BT_ADDR_LE_STR_LEN];
	uint8_t err;
	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
	char addrShort[18];
	bt_addr_le_to_str(device_info->recv_info->addr, addrShort, sizeof(addrShort));

	if (ready)
	{
		bt_scan_stop();
		// search first for the sensor 1 and then the sensor 2 and at the end sensor 3
		if (checkAddresses(addrShort,sensor1) && once_sensor1)
		{
			printk("Correct sensor found\n");
			once_sensor1 = false;
			err = bt_conn_le_create(device_info->recv_info->addr,
									BT_CONN_LE_CREATE_CONN,
									device_info->conn_param, &centralConnections[nbrConnectionsCentral]);
		}
		else if (checkAddresses(addrShort,sensor2) && once_sensor2 && !once_sensor1)
		{
			printk("Correct sensor found\n");
			once_sensor2 = false;
			err = bt_conn_le_create(device_info->recv_info->addr,
									BT_CONN_LE_CREATE_CONN,
									device_info->conn_param, &centralConnections[nbrConnectionsCentral]);
		}
		else if (checkAddresses(addrShort,sensor3) && once_sensor3)
		{
			printk("Correct sensor found\n");
			once_sensor3 = false;
			bt_scan_stop();
			err = bt_conn_le_create(device_info->recv_info->addr,
									BT_CONN_LE_CREATE_CONN,
									device_info->conn_param, &centralConnections[nbrConnectionsCentral]);
		}
		else 
		{
			startScan();
		}
	}
}

void DeviceManager::scanConnectionError(struct bt_scan_device_info *device_info) 
{
    printk("Connecting failed\n");
	startScan();
}

void DeviceManager::scanFilterNoMatch(struct bt_scan_device_info *device_info, bool connectable)
{
	// not used in this project
	bt_scan_stop();
	initScan();
}

void DeviceManager::deviceFound(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
{
	initScan();
}

void DeviceManager::connected(struct bt_conn *conn, uint8_t err) 
{
	bt_conn_info info;
	uint8_t error = bt_conn_get_info(conn,&info);
	if (error)
	{
		printk("Cannot get info of connection object\n");
		return;
	}
	if (info.role == BT_CONN_ROLE_MASTER)	// master -> central role
	{
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		if (err) 
		{
			printk("Failed to connect to %s (%u)\n", addr, err);
			startScan();
			return;
		}

		printk("Connected: %s\n", addr);

		// save connection at the first free place in the array
		for (uint8_t i=0; i <= MAX_CONNECTIONS_CENTRAL-1; i++)
		{
			if (centralConnections[i] == nullptr)
			{
				centralConnections[i] = bt_conn_ref(conn);
				break;
			}		 
		}

		bt_conn_unref(conn);
		nbrConnectionsCentral++;

		if (cscDisconnected)
		{
			cscDisconnected = false;
		}

		// discover service 
		switch (sensorInfos)
		{
		case 1:
			discoverCSC();
			break;
		case 2:
			discoverCSC();
			break;
		case 3:
			discoverCSC();
			break;
		case 4:
			if (nbrConnectionsCentral <= 2)
			{
				discoverCSC();
			}
			else
			{
				discoverHR();	
			}
			break;
		case 5:
			if (nbrConnectionsCentral == 1)
			{
				discoverCSC();
			}
			else
			{
				discoverHR();
			}
			break;
		case 6:
			if (nbrConnectionsCentral == 1)
			{
				discoverCSC();
			}
			else
			{
				discoverHR();
			}
			break;			
		case 7:
			discoverHR();
			break;
		default:
			break;
		}
	}
	else if (info.role == BT_CONN_ROLE_SLAVE)	// slave -> peripheral role
	{
		if (err) 
		{
			printk("Connection failed (err %u)\n", err);
			return;
		}
		disconnectOnce = true;
		connectedPeripheral = true;
		printk("Connected with application\n");
		peripheralConn = bt_conn_ref(conn);
		bt_conn_unref(conn);
		dk_set_led_on(CON_STATUS_LED_PERIPHERAL);			

		// when its in central and peripheral mode -> begin scanning
		if (getDevice() == 3 && nbrConnectionsCentral == 0) 
		{
			initScan();
		}	
	}	
}

void DeviceManager::disconnected(struct bt_conn *conn, uint8_t reason) 
{
	bt_conn_info info;
	uint8_t err = bt_conn_get_info(conn,&info);
	uint8_t disconnectedCode[1];
	uint8_t typeToReconnect = 0;

	if (err)
	{
		printk("Cannot get info of connection object\n");
		return;
	}

	if (info.role == BT_CONN_ROLE_SLAVE)	// slave -> peripheral role
	{
		peripheralDisconnected = true;
		connectedPeripheral = false;
		setDiameter(0);
		printk("Disconnected from Application (reason %u)\n", reason);		
		dk_set_led_off(CON_STATUS_LED_PERIPHERAL);
		startAdvertising();
	}
	else if (info.role == BT_CONN_ROLE_MASTER)	// master -> central role
	{
		char addr[BT_ADDR_LE_STR_LEN];
		char addrToFind[18];
		subscriptionDone = false;
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		printk("Disconnected from Sensor: %s (reason 0x%02x)\n", addr, reason);
		
		if (checkAddresses(addr,sensor1))
		{
			once_sensor1 = true;
			subscriptionDone = false;
			dk_set_led_off(CON_STATUS_LED_CENTRAL);
			if (sensorInfos == 7)
			{
				// disconnected from heart rate sensor
				hrDisconnected = true;
				reconnectedHeartRate = true;
				typeToReconnect = TYPE_HEARTRATE;
				if (!serviceNotFound)	// don't show disconnected message to user when service not found
				{
					disconnectedCode[0] = 13;
					data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
				}
				else
				{
					serviceNotFound = false;
				}		
			}
			else 
			{
				if (sensorInfos == 6 || sensorInfos == 2)
				{
					// cadence sensor disconneted
					cscDisconnected = true;
					typeToReconnect = TYPE_CSC_CADENCE;
					if (!serviceNotFound)	// don't show disconnected message to user when service not found
					{
						disconnectedCode[0] = 12;
						data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
					}
					else
					{
						serviceNotFound = false;
					}						
				}
				else
				{
					// speed sensor disconnected 
					cscDisconnected = true;
					typeToReconnect = TYPE_CSC_SPEED;
					if (!serviceNotFound)	// don't show disconnected message to user when service not found
					{
						disconnectedCode[0] = 11;
						data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
					}
					else
					{
						serviceNotFound = false;
					}						
				}
			}			
		}

		if (checkAddresses(addr,sensor2))
		{
			once_sensor2 = true;
			subscriptionDone = false;
			dk_set_led_off(CON_STATUS_LED_CENTRAL);
			if (sensorInfos == 3 || sensorInfos == 4)	
			{
				// cadence sensor disconnected
				cscDisconnected = true;
				typeToReconnect = TYPE_CSC_CADENCE;
				if (!serviceNotFound)	// don't show disconnected message to user when service not found
				{
					disconnectedCode[0] = 12;
					data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
				}
				else
				{
					serviceNotFound = false;
				}			
			}
			else
			{
				hrDisconnected = true;
				reconnectedHeartRate = true;
				typeToReconnect = TYPE_HEARTRATE;
				if (!serviceNotFound)	// don't show disconnected message to user when service not found
				{
					disconnectedCode[0] = 13;
					data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
				}
				else
				{
					serviceNotFound = false;
				}			
			}
		}

		if (checkAddresses(addr,sensor3))
		{
			hrDisconnected = true;
			reconnectedHeartRate = true;
			typeToReconnect = TYPE_HEARTRATE;
			subscriptionDone = false;
			once_sensor3 = true;
			dk_set_led_off(CON_STATUS_LED_CENTRAL);
			if (!serviceNotFound)	// don't show disconnected message to user when service not found
			{
				disconnectedCode[0] = 13;
				data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
			}
			else
			{
				serviceNotFound = false;
			}	
		}

		// delete the correct connection in the array
		for (uint8_t i = 0; i <= nbrConnectionsCentral-1; i++)
		{
			bt_addr_le_to_str(bt_conn_get_dst(centralConnections[i]), addrToFind, sizeof(addrToFind));

			if (checkAddresses(addr,addrToFind))
			{
				bt_conn_unref(centralConnections[i]);
				centralConnections[i] = nullptr;
				nbrConnectionsCentral--;
			}
		}

		// start scanning again -> search for the same sensor type which has disconnected
		reScan(typeToReconnect);
	}
}

bool DeviceManager::le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	return true;
}

void DeviceManager::le_param_updated(struct bt_conn *conn, uint16_t interval,
				 uint16_t latency, uint16_t timeout)
{}

void DeviceManager::discoverCSC()
{
	printk("nbr conn: %d\n", nbrConnectionsCentral);
	uint8_t err = bt_gatt_dm_start(centralConnections[nbrConnectionsCentral-1], BT_UUID_CSC, &discovery_cb_CSC, NULL);
	if (err) 
	{
		printk("Could not start service discovery, err %d\n", err);
	}
}

void DeviceManager::discoverHR()
{
	uint8_t err = bt_gatt_dm_start(centralConnections[nbrConnectionsCentral-1], BT_UUID_HRS, &discovery_cb_HR, NULL);
	if (err) 
	{
		printk("Could not start service discovery, err %d\n", err);
	}
}

void DeviceManager::discoveryCompletedCSC(struct bt_gatt_dm *dm, void *ctx) 
{
	uint8_t err;
	uint8_t connectedCode[1];
	if (!subscriptionDone)
	{	
		subscribe_params[nbrConnectionsCentral-1].value = BT_GATT_CCC_NOTIFY;
		subscribe_params[nbrConnectionsCentral-1].notify = onReceived;

		const struct bt_gatt_dm_attr *chrc;
		const struct bt_gatt_dm_attr *desc;

		// Get the characteristic by its UUID
		chrc = bt_gatt_dm_char_by_uuid(dm,BT_UUID_CSC_MEASUREMENT);
		if (!chrc) 
		{
			printk("Missing CSC measurement characteristic\n");
			err = bt_gatt_dm_data_release(dm);
			if (err) 
			{
				printk("Could not release discovery data, err: %d\n", err);
			}
			return;
		}

		// Search the descriptor by its UUID
		desc = bt_gatt_dm_desc_by_uuid(dm, chrc, BT_UUID_CSC_MEASUREMENT);
		if (!desc) 
		{
			printk("Missing CSC measurement char CCC descriptor\n");
			err = bt_gatt_dm_data_release(dm);
			if (err) 
			{
				printk("Could not release discovery data, err: %d\n", err);
			}
			return;
		}

		subscribe_params[nbrConnectionsCentral-1].value_handle = desc->handle;

		// Search the CCC descriptor by its UUID
		desc = bt_gatt_dm_desc_by_uuid(dm, chrc, BT_UUID_GATT_CCC);
		if (!desc) 
		{
			printk("Missing CSC measurement char CCC descriptor\n");
			err = bt_gatt_dm_data_release(dm);
			if (err) 
			{
				printk("Could not release discovery data, err: %d\n", err);
			}
			return;
		}

		subscribe_params[nbrConnectionsCentral-1].ccc_handle = desc->handle;
		
		// Subscribe attribute value notification
		err = bt_gatt_subscribe(bt_gatt_dm_conn_get(dm), &subscribe_params[nbrConnectionsCentral-1]);
		if (err) 
		{
			printk("Subscription failed (err %d)\n", err);
		}

		bt_gatt_dm_data_release(dm);
	}
	
	// check number of connections -> can be modified for more devices
	// send message code to client
	switch (nbrConnectionsCentral)
	{
	case 1:
		if (nbrAddresses == 1 && sensorInfos == 1)
		{
			printk("Discovery completed\n");
			subscriptionDone = true;
			dk_set_led_on(CON_STATUS_LED_CENTRAL);
			connectedCode[0] = 14;
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
		}
		else if (nbrAddresses == 1 && sensorInfos == 2)
		{
			printk("Discovery completed\n");
			subscriptionDone = true;
			dk_set_led_on(CON_STATUS_LED_CENTRAL);
			connectedCode[0] = 15;
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
		}
		else if (nbrAddresses == 2 && (sensorInfos == 3 || sensorInfos == 5))	
		{
			printk("First discovery completed\n");
			connectedCode[0] = 17;	// speed connected
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
			initScan();				
		}
		else if (nbrAddresses == 2 && sensorInfos == 6)
		{
			printk("First discovery completed\n");
			connectedCode[0] = 18;	// cadence connected
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
			initScan();				
		}
		else if (nbrAddresses == 3)
		{
			printk("First discovery completed\n");	
			connectedCode[0] = 17; // speed sensor connected
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
			initScan();
		}
		break;
	case 2:
		if (nbrAddresses == 2 && sensorInfos == 3)
		{
			printk("Second discovery completed\n");
			connectedCode[0] = 19;	// cadence sensor connected
			data_service_send(peripheralConn, connectedCode, sizeof(connectedCode));
			dk_set_led_on(CON_STATUS_LED_CENTRAL);
			subscriptionDone = true;
		}	
		else if (nbrAddresses == 3) 
		{
			printk("Second discovery completed\n");	
			connectedCode[0] = 21;	// cadence sensor connected
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
			initScan();
		}
		break;
	default:
		break;
	}
}

void DeviceManager::discovery_service_not_found(struct bt_conn *conn, void *ctx) 
{
	printk("Service not found!\n");
	serviceNotFound = true;
	uint8_t error[1];
	error[0] = 10;
	data_service_send(peripheralConn,error, sizeof(error));
	// reconnect for another try
	bt_conn_disconnect(conn,100);
}

void DeviceManager::discovery_error_found(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}


void DeviceManager::discoveryCompletedHR(struct bt_gatt_dm *dm, void *ctx) 
{
	uint8_t connectedCode[1];
	uint8_t err;

	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	// Get the characteristic by its UUID
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_HRS_MEASUREMENT);

	if (!gatt_chrc) 
	{
		printk("No heart rate measurement characteristic found\n");
		return;
	}

	// Search the descriptor by its UUID
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_HRS_MEASUREMENT);

	if (!gatt_desc) 
	{
		printk("No heat rate measurement characteristic value found\n");
		return;
	}

	subscribe_params[nbrConnectionsCentral-1].value_handle = gatt_desc->handle;

	// Search the CCC descriptor by its UUID
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
			BT_UUID_GATT_CCC);

	if (!gatt_desc) 
	{
		printk("No heart rate CCC descriptor found. "
		       "Heart rate service does not support notifications. \n");
		return;
	}

	subscribe_params[nbrConnectionsCentral-1].notify = notify_HR;
	subscribe_params[nbrConnectionsCentral-1].value = BT_GATT_CCC_NOTIFY;
	subscribe_params[nbrConnectionsCentral-1].ccc_handle = gatt_desc->handle;

	// Subscribe attribute value notification
	err = bt_gatt_subscribe(conn, &subscribe_params[nbrConnectionsCentral-1]);

	if (err && err != -EALREADY) 
	{
		printk("Subscribe failed (err %d)\n", err);
	} 
	else 
	{
		printk("[SUBSCRIBED]\n");
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) 
	{
		printk("Could not release the discovery data (err %d)\n", err);
	}

	subscriptionDone = true;

	// send message code to client
	switch (nbrConnectionsCentral)
	{
	case 1:
		dk_set_led_on(CON_STATUS_LED_CENTRAL);
		connectedCode[0] = 16;
		data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
		printk("Discovery completed\n");
		break;
	case 2:
		printk("Second discovery completed\n");
		dk_set_led_on(CON_STATUS_LED_CENTRAL);
		if (sensorInfos == 5)
		{
			if (reconnectedHeartRate)
			{
				if (batterySubscriptionDone)
				{
					reconnectedHeartRate = false;
					connectedCode[0] = 24;
				}
				else
				{
					reconnectedHeartRate = false;
					connectedCode[0] = 22;
				}
			}
			else
			{
				connectedCode[0] = 22;	
			}
			
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
		}
		else if (sensorInfos == 6)
		{
			if (reconnectedHeartRate)
			{
				if (batterySubscriptionDone)
				{
					reconnectedHeartRate = false;
					connectedCode[0] = 24;
				}
				else
				{
					reconnectedHeartRate = false;
					connectedCode[0] = 20;
				}
			}
			else
			{
				connectedCode[0] = 20;
			}

			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
		}
		break;
	case 3:
		if (reconnectedHeartRate)
		{
			if (batterySubscriptionDone)
			{
				reconnectedHeartRate = false;
				connectedCode[0] = 24;
			}
			else
			{
				reconnectedHeartRate = false;
				connectedCode[0] = 23;
			}	
		}
		else
		{
			connectedCode[0] = 23;
		}

		data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
		printk("Third discovery completed\n");
		dk_set_led_on(CON_STATUS_LED_CENTRAL);
		break;
	default:
		break;
	}
	dk_set_led_on(CON_STATUS_LED_CENTRAL);
}

uint8_t DeviceManager::onReceived(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length) 
{
	// local variables 
	uint8_t batteryLevelToSend[4];
	static uint8_t cntFirstSpeed = 0;
	static uint8_t cntFirstCadence = 0;
	static uint8_t cntNbrReceived1 = 0;
	static uint8_t cntNbrReceived2 = 0;
	static uint8_t cntForDiscover = 0;
	static uint8_t cntZerosSpeed = 0;
	static uint8_t cntZerosCadence = 0;
	uint8_t err = 0;
		
	// start calculating and showing data only when all characteristics are subscribed
	if (subscriptionDone)
	{
		if (!batterySubscriptionDone)
		{			
			if (cntBatterySubscriptions == nbrConnectionsCentral)
			{
				batterySubscriptionDone = true;
			}
			else 
			{
				if (isFree())
				{
					initBatteryManager(sensorInfos);
					err = gatt_discover_battery_service(centralConnections[cntBatterySubscriptions]);	
					
					printk("Nbr connections %d\n", cntBatterySubscriptions);

					if (err == 0)
					{
						cntBatterySubscriptions++;
					}
				}
			}
		}
		else if (!serviceFound())
		{
			cntBatterySubscriptions--;
			batterySubscriptionDone = false;
		}
		else
		{
			if (length > 0)
			{
				// when a sensor disconnects, ask for battery level
				if (cscDisconnected)
				{
					cntFirstSpeed = 0;
					cntFirstCadence = 0;			
				}

				// when application disconnects and reconnects, ask for battery level
				if (peripheralDisconnected && connectedPeripheral)
				{
					peripheralDisconnected = false;
					cntFirstSpeed = 0;
					cntFirstCadence = 0;
					cntFirstHR = 0;
				}
				
				// check if notifications are on, when no, disconnect from application -> so the user can reconnect
				if (!areNotificationsOn() && disconnectOnce)
				{
					disconnectOnce = false;
					bt_conn_disconnect(peripheralConn,1);
				}

				// save the new received data
				DeviceManager::data.saveData(data);

				uint8_t val_after_comma;
				uint8_t dataToSend[3];

				if (getDiameter() != 0 && diameterSet == false)
				{
					diameterSet = true;
					DeviceManager::data.wheelDiameter = getDiameter();
				}
				else if (getDiameter() == 0 && diameterSet == true)
				{
					// reset button was pressed
					diameterSet = false;
				}
				
				if (DeviceManager::data.type == TYPE_CSC_SPEED)
				{
					// calculate speed
					if (diameterSet)
					{						
						uint16_t speed = DeviceManager::data.calcSpeed();
						if (speed == 0)
						{
							cntZerosSpeed++;
						}
						else
						{
							cntZerosSpeed = 0;
						}

						if (speed > 0 || cntZerosSpeed >= 3)	// when 3 times speed is 0, bike is not running any more
						{
							// 1. value: type -> speed
							// 2. value: 8 bit on the left side of comma
							// 3. value: 8 bit on the right side of comma
							dataToSend[0] = TYPE_CSC_SPEED;
							dataToSend[1] = (uint8_t) (speed/100);	
							val_after_comma = (uint8_t) (speed);
							dataToSend[2] = val_after_comma;

							if (peripheralConn != nullptr)
							{	
								printk("Speed: %d\n",speed/100);
								data_service_send(peripheralConn,dataToSend, sizeof(dataToSend));
							}
						}
					}

					// ask from at the beginning and time to time for the battery level
					if (cntFirstSpeed == 2 || cntNbrReceived1 == 50)
					{
						cntNbrReceived1 = 0;
						askForBatteryLevel(TYPE_CSC_SPEED);
						cntFirstSpeed++;
					}
					else if (isValueReady(TYPE_CSC_SPEED))
					{
						// send new battery level to client
						resetReadyValue(TYPE_CSC_SPEED);
						DeviceManager::data.battValue_speed = getBatteryLevel(TYPE_CSC_SPEED);
						batteryLevelToSend[0] = TYPE_BATTERY;
						batteryLevelToSend[1] = TYPE_CSC_SPEED;
						batteryLevelToSend[2] = DeviceManager::data.battValue_speed;
						data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));				
					}
					else
					{
						cntNbrReceived1++;
						cntFirstSpeed++;
					}		
				}
				else if (DeviceManager::data.type == TYPE_CSC_CADENCE)
				{
					// calculate rpm (rounds per minute)
					uint16_t rpm = DeviceManager::data.calcRPM();
					if (rpm == 0)
					{	
						cntZerosCadence++;
					}
					else
					{
						cntZerosCadence = 0;
					}
					
					if (rpm > 0 || cntZerosCadence >= 3)	// when 3 times speed is 0, bike is not running any more
					{			
						if (rpm < 500)
						{
							// 1. value: type -> cadence
							// 2. value: 8 lsb of cadence value
							// 3. value: 8 msb of cadence value					
							dataToSend[0] = TYPE_CSC_CADENCE;	
							dataToSend[1] = (uint8_t) rpm;
							dataToSend[2] = (uint8_t) (rpm >> 8);	
							if (peripheralConn != nullptr)
							{
								printk("Cadence rpm: %d\n",rpm);
								data_service_send(peripheralConn,dataToSend, sizeof(dataToSend));
							}
						}
					}	

					// ask from time to time for the battery level
					if (cntFirstCadence == 3 || cntNbrReceived2 == 100)
					{
						askForBatteryLevel(TYPE_CSC_CADENCE);
						cntNbrReceived2 = 0;
						cntFirstCadence++;
					}
					else if (isValueReady(TYPE_CSC_CADENCE))
					{
						// send new battery level to client
						resetReadyValue(TYPE_CSC_CADENCE);
						DeviceManager::data.battValue_cadence = getBatteryLevel(TYPE_CSC_CADENCE);
						batteryLevelToSend[0] = TYPE_BATTERY;
						batteryLevelToSend[1] = TYPE_CSC_CADENCE;	
						batteryLevelToSend[2] = DeviceManager::data.battValue_cadence;
						data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));			
					}
					else 
					{
						cntFirstCadence++;
						cntNbrReceived2++;
					}					
				}
			}
		}
	}
	else
	{
		cntFirstSpeed = 0;
		cntFirstCadence = 0;		
	}

	return BT_GATT_ITER_CONTINUE;
}

uint8_t DeviceManager::notify_HR(struct bt_conn *conn,
		struct bt_gatt_subscribe_params *params,
		const void *data, uint16_t length) 
{
	// local variables
	uint8_t err = 0;
	static bool onceHeartRate = true;
	static uint16_t cntNbrReceived = 0;	
	uint8_t dataToSend[2];
	uint8_t batteryLevelToSend[4];
	dataToSend[0] = TYPE_HEARTRATE;
	batteryLevelToSend[0] = TYPE_BATTERY;
	batteryLevelToSend[1] = TYPE_HEARTRATE;

	if (sensorInfos == 7)
	{
		if (onceHeartRate || (peripheralDisconnected && connectedPeripheral))
		{
			cntFirstHR = 0;
			peripheralDisconnected = false;
			onceHeartRate = false;
			initBatteryManager(sensorInfos);
			err = gatt_discover_battery_service(centralConnections[cntBatterySubscriptions]);	
			if (err == 0)
			{
				batterySubscriptionDone = true;
			}			
		}	
	}

	if (hrDisconnected)
	{
		hrDisconnected = false;
		cntFirstHR = 0;
	}
	
	if (batterySubscriptionDone)
	{
		// ask from time to time for the battery level
		if (cntFirstHR == 2 || cntNbrReceived == 300)
		{
			cntFirstHR++;
			cntNbrReceived++;
			askForBatteryLevel(TYPE_HEARTRATE);
		}
		else if (isValueReady(TYPE_HEARTRATE))
		{
			// send new battery level to client
			resetReadyValue(TYPE_HEARTRATE);
			cntNbrReceived = 0;
			cntFirstHR++;
			DeviceManager::data.battValue_heartRate = getBatteryLevel(TYPE_HEARTRATE);
			batteryLevelToSend[2] = DeviceManager::data.battValue_heartRate;
			data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));
		}
		else
		{
			cntFirstHR++;
			cntNbrReceived++;
		}	

		if (!data) {
			printk("[UNSUBSCRIBED]\n");
			params->value_handle = 0U;
			return BT_GATT_ITER_STOP;
		}

		if (length == 2)
		{
				uint8_t hr_bpm = ((uint8_t *)data)[1];
				DeviceManager::data.heartRate = hr_bpm;
				dataToSend[1] = hr_bpm;
				printk("[NOTIFICATION] Heart Rate %u bpm\n", hr_bpm);
				data_service_send(peripheralConn,dataToSend,sizeof(dataToSend));
		} 
		else 
		{
			printk("[NOTIFICATION] data %p length %u\n", data, length);
		}
	}
	else
	{
		cntFirstHR = 0;
	}

	return BT_GATT_ITER_CONTINUE;
}

bool DeviceManager::checkAddresses(char addr1[],char addr2[])
{
	uint8_t cnt = 0;
	bool retVal = false;
	for (uint8_t i = 0; i < 17; i++)
	{
		if (addr1[i] == addr2[i])
		{
			cnt++;
		}
	}
	if (cnt == 17)
	{
		cnt = 0;
		retVal = true;
	}
	else
	{
		cnt = 0;
		retVal = false;
	}
	return retVal;
}
