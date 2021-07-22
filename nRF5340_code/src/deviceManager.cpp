#include "deviceManager.h"

// defines

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)


#define CON_STATUS_LED_PERIPHERAL       DK_LED1
#define CON_STATUS_LED_CENTRAL          DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000

#define USER_LED                DK_LED4

#define USER_BUTTON             DK_BTN1_MSK


/* Thingy advertisement UUID */
#define BT_UUID_THINGY                                                         \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x01, 0x68, 0xEF)

/* Thingy service UUID */
#define BT_UUID_UI                                                         \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x03, 0x68, 0xEF)

/* Thingy button characteristic UUID */
#define BT_UUID_BUTTON                                                     \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x02, 0x03, 0x68, 0xEF)

/* Thingy led characteristic UUID */
#define BT_UUID_LED                                                        \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x01, 0x03, 0x68, 0xEF)

// Type definitions
#define TYPE_CSC_SPEED 1
#define TYPE_CSC_CADENCE 2
#define TYPE_HEARTRATE 3
#define TYPE_BATTERY 4

BT_GATT_SERVICE_DEFINE(csc_srv,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CSC),
	BT_GATT_CHARACTERISTIC(BT_UUID_CSC_MEASUREMENT, BT_GATT_CHRC_NOTIFY,
			       0x00, NULL, NULL, NULL),			   
);

/*
 * data struct advertising
 */  
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/*
 * data struct scanning
 */  
static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

static struct bt_gatt_subscribe_params param = {
	//.notify = onReceived,
	.value = BT_GATT_CCC_NOTIFY,
};

static struct devices{
    int CENTRAL;
    int PERIPHERAL;
    int CENTRAL_PERIPHERAL;
}device;

static char sensor1[17]; 
static char sensor2[17];
static char sensor3[17];

// initalize static attributes
bool deviceManager::isCentral = false;
bool deviceManager::isPeripheral = false;
bool deviceManager::connectedC = false;
bool deviceManager::connectedP = false;
bool deviceManager::app_button_state = false;
bool deviceManager::subscriptionDone = false;
bool deviceManager::diameterSet = false;
bool deviceManager::once_sensor1 = true;
bool deviceManager::once_sensor2 = true;
bool deviceManager::once_sensor3 = true;
bool deviceManager::batterySubscriptionDone = false;
bool deviceManager::wasDisconnected = false;
uint8_t deviceManager::nbrAddresses = 0;
uint8_t deviceManager::nbrConnectionsCentral = 0;
uint8_t deviceManager::sensorInfos = 0;
uint8_t deviceManager::cntBatterySubscriptions = 0;
bt_conn* deviceManager::peripheralConn;
bt_conn* deviceManager::centralConnections[];
dataCSC deviceManager::data;
//BatteryManager deviceManager::battManager;
//bt_bas_client deviceManager::bas;

static struct bt_gatt_dm_cb discovery_cb = 
{
	.completed = deviceManager::discoveryCompletedCSC,
	.service_not_found = deviceManager::discovery_service_not_found,
	.error_found = deviceManager::discovery_error_found,
};

static struct bt_gatt_dm_cb discovery_cb_HR = 
{
	.completed = deviceManager::discoveryCompletedHR,
	.service_not_found = deviceManager::discovery_service_not_found,
	.error_found = deviceManager::discovery_error_found,
};


/*-----------------------------------------------------------------------------------------------------
 * GENERAL METHODS
 *---------------------------------------------------------------------------------------------------*/

deviceManager::deviceManager(){
    device.CENTRAL = 1;
    device.PERIPHERAL = 2;
    device.CENTRAL_PERIPHERAL = 3;

	for (size_t i = 0; i < MAX_CONNECTIONS_CENTRAL-1; i++)
	{
		centralConnections[i] = nullptr;
	}

	connectedP = false;
	connectedC = false;
}

uint8_t deviceManager::getDevice(){
    if(isCentral && isPeripheral){
        return 3;
    } else if (!isCentral && isPeripheral){
        return 2;
    } else if (isCentral && !isPeripheral){
        return 1;
    } else {
        return 0;
    }
}

void deviceManager::setDevice(bool c, bool p){
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

void deviceManager::app_led_cb(bool led_state){
	// set led to led_state
    dk_set_led(USER_LED,led_state);
}

bool deviceManager::app_button_cb(void){
    return app_button_state;
}

void deviceManager::buttonChanged(uint32_t button_state, uint32_t has_changed){
   	if (has_changed & USER_BUTTON) {
		bt_lbs_send_button_state(button_state);
		app_button_state = button_state ? true : false;
	} 
}

int deviceManager::initButton(){
    int err;

	// initialize method buttonChanged as callback when state of button changes
    err = dk_buttons_init(buttonChanged);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

    return err;
}

/*-----------------------------------------------------------------------------------------------------
 * PERIPHERAL ROLE
 *---------------------------------------------------------------------------------------------------*/

void deviceManager::initPeripheral(){
    int err;
    if(getDevice() == 3 || getDevice() == 2){

		// initialize leds
        err = dk_leds_init();
        if (err) {
            printk("LEDs init failed (err %d)\n", err);
            return;
        }

		// initialize buttons
        err = initButton();
        if (err) {
            printk("Button init failed (err %d)\n", err);
            return;
        }

		// enable bluetooth
        err = bt_enable(NULL);
        if (err) {
            printk("Bluetooth init failed (err %d)\n", err);
            return;
        }

        printk("Bluetooth initialized\n");

		// register callback functions
		bt_conn_cb_register(&conn_callbacks);

		// config settings
        if (IS_ENABLED(CONFIG_SETTINGS)) {
            settings_load();
        }

		// initialize Led Button Service
        err = bt_lbs_init(&lbs_callbacs);
        if (err) {
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

void deviceManager::startAdvertising() {
	int err;
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("Advertising successfully started\n");
	printk("Waiting for connection...\n");
}

/*-----------------------------------------------------------------------------------------------------
 * CENTRAL ROLE
 *---------------------------------------------------------------------------------------------------*/
 
void deviceManager::initCentral(){
	printk("Init Central\n");
	if (getDevice() == 1 || getDevice() == 3)
	{
	    int err;
		if (getDevice() != 3)
		{
			// enable bluetooth
			err = bt_enable(nullptr);
			if (err) {
				printk("Bluetooth init failed (err %d)\n", err);
				return;
			}
			printk("Bluetooth ready\n");

			// initialize leds
			err = dk_leds_init();
			if (err) {
				printk("LEDs init failed (err %d)\n", err);
				return;
			}

			// initialize buttons
			err = initButton();
			if (err) {
				printk("Button init failed (err %d)\n", err);
				return;
			}

			// initialize Led Button Service
			err = bt_lbs_init(&lbs_callbacs);
			if (err) {
				printk("Failed to init LBS (err:%d)\n", err);
				return;
			}

			// config settings
			if (IS_ENABLED(CONFIG_SETTINGS)) {
				settings_load();
				printk("Settings loaded\n");
			}
			
		}

		bt_conn_cb_register(&conn_callbacks);
		initScan();
		startScan();	
	}
}

void deviceManager::initScan() {
	int err;
	static bool once = true;
	sensorInfos = getSensorInfos();
	
	struct bt_le_scan_param scanParam = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
        .timeout = 0
    };	

	struct bt_scan_init_param scanInit = {
		.scan_param = &scanParam,
		.connect_if_match = 0,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT,
	};

	if (sensorInfos != 0)
	{
		if (once)
		{
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
				// search just 1 CSC sensor
				// add CSC UUID filter
				err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
				if (err) {
					printk("Scanning filters cannot be set\n");
					return;
				}
				break;
			case 2:
				// search just for CSC sensors
				// add CSC UUID filter
				err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
				if (err) {
					printk("Scanning filters cannot be set\n");
					return;
				}
				break;
			case 3:
				// first search 2 CSC sensors, after a heart rate sensor
				if (nbrConnectionsCentral == 0 || nbrConnectionsCentral == 1)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
					if (err) {
						printk("Scanning filters cannot be set\n");
						return;
					}
				}
				else if (nbrConnectionsCentral == 2)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
					if (err) {
						printk("Scanning filters cannot be set\n");
						return;
					}
				}
				break;
			case 4:
				// first search for a CSC sensor, after a heart rate sensor
				if (nbrConnectionsCentral == 0)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
					if (err) {
						printk("Scanning filters cannot be set\n");
						return;
					}				
				}
				else if (nbrConnectionsCentral == 1)
				{
					err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
					if (err) {
						printk("Scanning filters cannot be set\n");
						return;
					}
				}
				break;
			case 5:
				// just a heart rate sensor to connect
				err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
				if (err) {
					printk("Scanning filters cannot be set\n");
					return;
				}	
				break;	
			default:
				break;
		}
		
		// enable filters
		err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
		if (err) {
			printk("Filters cannot be turned on\n");
		}
		startScan();
	} 
	else 
	{
		err = bt_le_scan_start(&scanParam, deviceFound);
		if (err) {
			//printk("Scanning failed to start (err %d)\n", err);
		}
	}
}

void deviceManager::startScan(){
    int err;
	
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start, err %d\n", err);
	}
	printk("Scanning...\n");
}

void deviceManager::scanFilterMatch(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable) {

	static bool ready = false;
	static bool CSCDone = false;
	nbrAddresses = getNbrOfAddresses();
	
	if (nbrAddresses != 0)
	{
		ready = true;
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

	//printk("Filters matched. Address: %s connectable: %s\n",
	//	addr, connectable ? "yes" : "no");

	if (ready)
	{
		bt_scan_stop();

		if (nbrConnectionsCentral == 1 && sensorInfos == 4)
		{
			CSCDone = true;
		}
		else if (nbrConnectionsCentral == 2 && sensorInfos == 3)
		{
			CSCDone = true;
		}
		else if (sensorInfos == 1 || sensorInfos == 2)
		{
			CSCDone = true;
		}
		
		if (checkAddresses(addrShort,sensor1) && once_sensor1)
		{
			printk("Correct sensor found\n");
			once_sensor1 = false;
			err = bt_conn_le_create(device_info->recv_info->addr,
									BT_CONN_LE_CREATE_CONN,
									device_info->conn_param, &centralConnections[nbrConnectionsCentral]);
		}
		else if (checkAddresses(addrShort,sensor2) && once_sensor2)
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

/*
 * callback function
 * when while connecting an error appears
 */
void deviceManager::scanConnectionError(struct bt_scan_device_info *device_info) {
    printk("Connecting failed\n");
	startScan();
}

void deviceManager::scanFilterNoMatch(struct bt_scan_device_info *device_info, bool connectable) {
	bt_scan_stop();
	int err = 0;
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(device_info->recv_info->addr, addr,
				  sizeof(addr));
	//printk("Filters not matched. Address: %s \n", addr);

	if (checkAddresses(addr,sensor1) && once_sensor1)
	{
		printk("Correct sensor found\n");
		once_sensor1 = false;
		err = bt_conn_le_create(device_info->recv_info->addr,
								BT_CONN_LE_CREATE_CONN,
								device_info->conn_param, &centralConnections[nbrConnectionsCentral]);
	}
	else if (checkAddresses(addr,sensor2) && once_sensor2)
	{
		if ((nbrConnectionsCentral == 1 && sensorInfos == 3) || (nbrConnectionsCentral == 1 && sensorInfos == 4))
		{
			printk("Correct sensor found\n");
			once_sensor2 = false;
			err = bt_conn_le_create(device_info->recv_info->addr,
									BT_CONN_LE_CREATE_CONN,
									device_info->conn_param, &centralConnections[nbrConnectionsCentral]);
		}	
	}	

	initScan();
}

void deviceManager::deviceFound(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad) {
	//printk("init scan\n");
	initScan();
	/*char addr_str[BT_ADDR_LE_STR_LEN];
	char addr_toConnect[18] = "EA:1A:AD:15:54:9F";
	int err = 0;
	static int cnt = 0;

	// We're only interested in connectable events 
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	// connect only to devices in close proximity 
	if (rssi < -70) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device Found: %s\n",addr_str);
	cnt++;
	printk("Count devices %i\n",cnt);
	if(strstr(addr_str,addr_toConnect)){
        struct bt_conn_le_create_param param = {
            .options = BT_CONN_LE_OPT_NONE,
            .interval = BT_GAP_SCAN_FAST_INTERVAL,
            .window = BT_GAP_SCAN_FAST_INTERVAL,
            .interval_coded = 0,
            .window_coded = 0,
            .timeout = 0,
        };
		printk("Device found: %s (RSSI %d)\n", addr_str, rssi);
        err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                BT_LE_CONN_PARAM_DEFAULT, &centralConnections[nbrConnectionsCentral-1]);
        if (err) {
            printk("Create conn to %s failed (%u)\n", addr_str, err);
            startScan();
        }
	}
    else{
        startScan();
    }   */
}

void deviceManager::connected(struct bt_conn *conn, uint8_t err) {
	bt_conn_info info;
	int error = bt_conn_get_info(conn,&info);
	if (error)
	{
		printk("Cannot get info of connection object\n");
		return;
	}
	if (info.role == BT_CONN_ROLE_MASTER)	// master -> central role
	{
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		if (err) {
			printk("Failed to connect to %s (%u)\n", addr, err);
			startScan();
			return;
		}

		printk("Connected: %s\n", addr);

		// save connection 
		centralConnections[nbrConnectionsCentral] = bt_conn_ref(conn);;
		bt_conn_unref(conn);
		nbrConnectionsCentral++;

		/*struct bt_le_conn_param param1 = BT_LE_CONN_PARAM_INIT(50,100,0,4000);
		err = bt_conn_le_param_update(centralConnections[nbrConnectionsCentral-1],&param1);
		if (err)
		{
			printk("Cannot change connection paramters: %s\n", err);
		}*/
		
		/*if (wasDisconnected)
		{
			error = bt_gatt_unsubscribe(conn,&param);
			if (error != 0)
			{
				printk("Cannot unsubscribe\n");
			}
		}*/
		

		//bt_gatt_dm_start(conn,BT_UUID_BAS,&discovery_cb,NULL);

		//if (!wasDisconnected)
		//{
			switch (sensorInfos)
			{
			case 1:
				discoverCSC();
				break;
			case 2:
				discoverCSC();
				break;
			case 3:
				if (nbrConnectionsCentral <= 2)
				{
					discoverCSC();
				}
				else
				{
					discoverHR();	
				}
				break;
			case 4:
				if (nbrConnectionsCentral == 1)
				{
					discoverCSC();
				}
				else
				{
					discoverHR();
				}
				break;
			case 5:
				discoverHR();
				break;
			default:
				break;
			}
		//}
		//else
		//{
		//	subscriptionDone = true;
		//}
		

	}
	else if (info.role == BT_CONN_ROLE_SLAVE)	// slave -> peripheral role
	{
		if (err) {
			printk("Connection failed (err %u)\n", err);
			return;
		}
		printk("Connected with application\n");
		connectedP = true;
		peripheralConn = bt_conn_ref(conn);
		bt_conn_unref(conn);
		dk_set_led_on(CON_STATUS_LED_PERIPHERAL);	
		

		// when its in central and peripheral mode -> begin scanning
		if (getDevice() == 3 && nbrConnectionsCentral == 0) 
		{
			initScan();
			//startScan();
		}
	}	
}

void deviceManager::disconnected(struct bt_conn *conn, uint8_t reason) {
	bt_conn_info info;
	int error = bt_conn_get_info(conn,&info);
	uint8_t disconnectedCode[1];

	if (error)
	{
		printk("Cannot get info of connection object\n");
		return;
	}
	if (info.role == BT_CONN_ROLE_SLAVE)	// slave -> peripheral role
	{
		printk("Disconnected from Application (reason %u)\n", reason);
		connectedP = false;
		//bt_conn_unref(peripheralConn);
		//peripheralConn = nullptr;
		
		dk_set_led_off(CON_STATUS_LED_PERIPHERAL);
		startAdvertising();
	}
	else if (info.role == BT_CONN_ROLE_MASTER)	// master -> central role
	{
		char addr[BT_ADDR_LE_STR_LEN];
		char addrToFind[18];
		if (reason == 8)
		{
			wasDisconnected = true;
		}
		cntBatterySubscriptions--;
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		printk("Disconnected from Sensor: %s (reason 0x%02x)\n", addr, reason);

		if (nbrConnectionsCentral == 0)
		{
			connectedC = false;
			disconnectedCode[0] = 13;
			data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
			subscriptionDone = false;
		}
		

		if (checkAddresses(addr,sensor1))
		{
			once_sensor1 = true;
			subscriptionDone = false;
			dk_set_led_off(CON_STATUS_LED_CENTRAL);
			if (sensorInfos == 5)
			{
				disconnectedCode[0] = 12;
				data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
			}
			else 
			{
				disconnectedCode[0] = 11;
				data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
			}
			/*error = bt_gatt_unsubscribe(conn,&param);
			if (error != 0)
			{
				printk("Cannot unsubscribe\n");
			}*/
			
		}

		if (checkAddresses(addr,sensor2))
		{
			once_sensor2 = true;
			subscriptionDone = false;
			dk_set_led_off(CON_STATUS_LED_CENTRAL);
			if (sensorInfos == 2 || sensorInfos == 3)
			{
				disconnectedCode[0] = 11;
				data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
			}
			else
			{
				disconnectedCode[0] = 12;
				data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
			}
			/*error = bt_gatt_unsubfscribe(conn,&param);
			if (error != 0)
			{
				printk("Cannot unsubscribe\n");
			}*/
		}

		if (checkAddresses(addr,sensor3))
		{
			subscriptionDone = false;
			once_sensor3 = true;
			dk_set_led_off(CON_STATUS_LED_CENTRAL);
			disconnectedCode[0] = 12;
			data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
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
		
		
		// start scanning again
		startScan();	
	}
}

bool deviceManager::le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	//printk("Accept new parameters\n");

	return true;
}

void deviceManager::le_param_updated(struct bt_conn *conn, uint16_t interval,
				 uint16_t latency, uint16_t timeout)
{
	//printk("Params updated!\n");
}

void deviceManager::discoverCSC()
{
	printk("nbr connection: %d\n",nbrConnectionsCentral);
	uint8_t err = bt_gatt_dm_start(centralConnections[nbrConnectionsCentral-1], BT_UUID_CSC, &discovery_cb, NULL);
	if (err) 
	{
		printk("Could not start service discovery, err %d\n", err);
	}
}

void deviceManager::discoverHR()
{
	uint8_t err = bt_gatt_dm_start(centralConnections[nbrConnectionsCentral-1], BT_UUID_HRS, &discovery_cb_HR, NULL);
	if (err) 
	{
		printk("Could not start service discovery, err %d\n", err);
	}
}

void deviceManager::discoveryCompletedCSC(struct bt_gatt_dm *dm, void *ctx) {
	int err;
	uint8_t connectedCode[1];
	if (!subscriptionDone)
	{	
		// subscribe CSC characteristic
		/*static struct bt_gatt_subscribe_params param = {
			.notify = onReceived,
			.value = BT_GATT_CCC_NOTIFY,
		};*/

		param.notify = onReceived;

		const struct bt_gatt_dm_attr *chrc;
		const struct bt_gatt_dm_attr *desc;

		// Get the characteristic by its UUID
		chrc = bt_gatt_dm_char_by_uuid(dm,BT_UUID_CSC_MEASUREMENT);
		if (!chrc) {
			printk("Missing CSC measurement characteristic\n");
			err = bt_gatt_dm_data_release(dm);
			if (err) {
				printk("Could not release discovery data, err: %d\n", err);
			}
			return;
		}

		// Search the descriptor by its UUID
		desc = bt_gatt_dm_desc_by_uuid(dm, chrc, BT_UUID_CSC_MEASUREMENT);
		if (!desc) {
			printk("Missing CSC measurement char CCC descriptor\n");
			err = bt_gatt_dm_data_release(dm);
			if (err) {
				printk("Could not release discovery data, err: %d\n", err);
			}
			return;
		}
		param.value_handle = desc->handle;

		// Search the CCC descriptor by its UUID
		desc = bt_gatt_dm_desc_by_uuid(dm, chrc, BT_UUID_GATT_CCC);
		if (!desc) {
			printk("Missing CSC measurement char CCC descriptor\n");
			err = bt_gatt_dm_data_release(dm);
			if (err) {
				printk("Could not release discovery data, err: %d\n", err);
			}
			return;
		}
		param.ccc_handle = desc->handle;
		//printk("Params value: %s\n", param.value);
		
		// Subscribe Attribute Value Notification
		err = bt_gatt_subscribe(bt_gatt_dm_conn_get(dm), &param);
		if (err) {
			printk("Subscribtion failed (err %d)\n", err);
		}
		bt_gatt_dm_data_release(dm);
	}
	
	// check number of connections -> can be modified for more devices
	switch (nbrConnectionsCentral)
	{
	case 1:
		if (nbrAddresses == 1)
		{
			connectedCode[0] = 14;
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
			printk("Discovery completed\n");
			subscriptionDone = true;
		}
		else if (nbrAddresses == 2)	
		{
			connectedCode[0] = 15;
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
			initScan();
			printk("First discovery completed\n");			
		}
		else if (nbrAddresses == 3)
		{
			initScan();
			printk("First discovery completed\n");	
		}
		
		break;
	case 2:
		if (nbrAddresses == 2)
		{
			connectedCode[0] = 16;
			data_service_send(peripheralConn, connectedCode, sizeof(connectedCode));
			printk("Second discovery completed\n");
			dk_set_led_on(CON_STATUS_LED_CENTRAL);
			connectedC = true;	
			subscriptionDone = true;
		}
		else if (nbrAddresses == 3) 
		{
			connectedCode[0] = 17;
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
			printk("Second discovery completed\n");		
			initScan();
		}
		break;
	case 3:
			connectedCode[0] = 18;
			data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
			printk("Third discovery completed\n");
			dk_set_led_on(CON_STATUS_LED_CENTRAL);
			connectedC = true;	
			subscriptionDone = true;		
		break;
	default:
		break;
	}
}


void deviceManager::discovery_service_not_found(struct bt_conn *conn, void *ctx) {
	printk("Service not found!\n");
	static uint8_t cnt = 0;
	uint8_t err;
	uint8_t error[1];
	error[0] = 10;
	data_service_send(peripheralConn,error, sizeof(error));
	bt_conn_disconnect(conn,-5);
	//discoverCSC();
}


void deviceManager::discovery_error_found(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}


void deviceManager::discoveryCompletedHR(struct bt_gatt_dm *dm, void *ctx) 
{
	int err;

	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	// subscribe CSC characteristic
	static struct bt_gatt_subscribe_params paramHR;

	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	/* Heart rate characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_HRS_MEASUREMENT);
	if (!gatt_chrc) {
		printk("No heart rate measurement characteristic found\n");
		return;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
			BT_UUID_HRS_MEASUREMENT);
	if (!gatt_desc) {
		printk("No heat rate measurement characteristic value found\n");
		return;
	}

	paramHR.value_handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
			BT_UUID_GATT_CCC);

	if (!gatt_desc) {
		printk("No heart rate CCC descriptor found. "
		       "Heart rate service does not support notifications. \n");
		return;
	}

	paramHR.notify = notify_HR;
	paramHR.value = BT_GATT_CCC_NOTIFY;
	paramHR.ccc_handle = gatt_desc->handle;

	err = bt_gatt_subscribe(conn, &paramHR);
	if (err && err != -EALREADY) {
		printk("Subscribe failed (err %d)\n", err);
	} else {
		printk("[SUBSCRIBED]\n");
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data (err %d)\n", err);
	}
	subscriptionDone = true;
}

uint8_t deviceManager::onReceived(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length) {

	static uint8_t cntSensors = 0;
	uint8_t batteryLevelToSend[4];
	static bool onceSpeed = true;
	static bool onceCadence = false;
	static uint8_t cntNbrReceived1 = 0;
	static uint8_t cntNbrReceived2 = 0;
	static uint8_t waitCnt = 0;
	// start calculating and showing data only when all characteristics are subscribed
	if (subscriptionDone)
	{
		if (!batterySubscriptionDone)
		{
			static uint8_t cnt2 = 0;
			uint8_t err = 0;
			if (cntBatterySubscriptions == nbrConnectionsCentral)
			{
				batterySubscriptionDone = true;
			}
			else 
			{
				if (cnt2 == 0 || cnt2 == 7 || cnt2 == 14) 
				{
					initBatteryManager(sensorInfos);
					err = gatt_discover_battery_service(centralConnections[cntBatterySubscriptions]);	
					if (err == 0)
					{
						cntBatterySubscriptions++;
					}
				}
				cnt2++;
			}
		}
		else 
		{
			if (length > 0)
			{
				// save the new received data
				deviceManager::data.saveData(data);

				uint8_t val_after_comma;
				uint8_t dataToSend[3];

				if (getDiameter() != 0 && diameterSet == false)
				{
					diameterSet = true;
					deviceManager::data.wheelDiameter = getDiameter();
				}
				else if (getDiameter() == 0 && diameterSet == true)
				{
					// reset button was pressed
					diameterSet = false;
				}
				
				if (deviceManager::data.type == TYPE_CSC_SPEED)
				{
					// calculate speed
					if (diameterSet)
					{
						uint16_t speed = deviceManager::data.calcSpeed();

						if (speed > 0)
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
					if (onceSpeed || cntNbrReceived1 == 50)
					{
						onceSpeed = false;
						cntNbrReceived1 = 0;
						deviceManager::data.battValue_speed = getBatteryLevel(TYPE_CSC_SPEED);
						batteryLevelToSend[0] = TYPE_BATTERY;
						batteryLevelToSend[1] = TYPE_CSC_SPEED;
						batteryLevelToSend[2] = deviceManager::data.battValue_speed;
						data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));
					}
					else if (waitCnt == 5)
					{
						onceCadence = true;
						waitCnt++;
					}
					else
					{
						cntNbrReceived1++;
						waitCnt++;
					}		
				}
				else if (deviceManager::data.type == TYPE_CSC_CADENCE)
				{
					// calculate rpm (rounds per minute)
					if (diameterSet)
					{
						uint16_t rpm = deviceManager::data.calcRPM();
						
						if (rpm > 0)
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
					if (onceCadence || cntNbrReceived2 == 100)
					{
						onceCadence = false;
						cntNbrReceived2 = 0;
						deviceManager::data.battValue_cadence = getBatteryLevel(TYPE_CSC_CADENCE);
						batteryLevelToSend[0] = TYPE_BATTERY;
						batteryLevelToSend[1] = TYPE_CSC_CADENCE;
						batteryLevelToSend[2] = deviceManager::data.battValue_cadence;
						data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));
					}
					else 
					{
						cntNbrReceived2++;
					}					
				}
				
				/*static uint8_t cntNbrReceived = 0;
				static uint8_t cntForStart = 0;
				static uint8_t waitToThisNumber = 200;
				if (cntForStart == 0 || cntForStart == 10 || cntNbrReceived == waitToThisNumber)
				{
					cntNbrReceived = 0;
					switch (cntSensors)
					{
					case 0:
						deviceManager::data.battValue_sensor1 = getBatteryLevel(cntSensors);
						batteryLevelToSend[0] = TYPE_BATTERY;
						batteryLevelToSend[1] = cntSensors;
						batteryLevelToSend[2] = deviceManager::data.battValue_sensor1;
						data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));
						break;
					case 1:
						deviceManager::data.battValue_sensor2 = getBatteryLevel(cntSensors);
						batteryLevelToSend[0] = TYPE_BATTERY;
						batteryLevelToSend[1] = cntSensors;
						batteryLevelToSend[2] = deviceManager::data.battValue_sensor2;
						data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));					
						break;
					case 2:
						deviceManager::data.battValue_sensor3 = getBatteryLevel(cntSensors);
						batteryLevelToSend[0] = TYPE_BATTERY;
						batteryLevelToSend[1] = cntSensors;
						batteryLevelToSend[2] = deviceManager::data.battValue_sensor3;
						data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));					
						break;				
					default:
						break;
					}

					if (cntSensors == nbrConnectionsCentral)
					{
						cntSensors = 0;
					}
					else
					{
						cntSensors++;
					}
				}
				cntForStart++;
				cntNbrReceived++;*/
			}
		}
		

	}
	return BT_GATT_ITER_CONTINUE;
}

uint8_t deviceManager::notify_HR(struct bt_conn *conn,
		struct bt_gatt_subscribe_params *params,
		const void *data, uint16_t length) {

	static bool onceHeartRate = true;
	static bool onceBatt = true;
	static uint16_t cntNbrReceived = 0;		
	uint8_t dataToSend[2];
	uint8_t batteryLevelToSend[4];
	dataToSend[0] = TYPE_HEARTRATE;
	uint8_t err = 0;

	if (sensorInfos == 5 && onceHeartRate)
	{
		onceHeartRate = false;
		initBatteryManager(sensorInfos);
		err = gatt_discover_battery_service(centralConnections[0]);	
		if (err)
		{
			printk("Error\n");
		}
		
	}

	// get battery level every few minutes
	if (onceBatt || cntNbrReceived == 300)
	{
		onceBatt = false;
		cntNbrReceived = 0;
		deviceManager::data.battValue_heartRate = getBatteryLevel(TYPE_HEARTRATE);
		batteryLevelToSend[0] = TYPE_BATTERY;
		batteryLevelToSend[1] = TYPE_HEARTRATE;
		batteryLevelToSend[2] = deviceManager::data.battValue_heartRate;
		data_service_send(peripheralConn,batteryLevelToSend,sizeof(batteryLevelToSend));
	}
	else
	{
		cntNbrReceived++;
	}	
	
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	if (length == 2) {
		if (batterySubscriptionDone)
		{
			uint8_t hr_bpm = ((uint8_t *)data)[1];
			deviceManager::data.heartRate = hr_bpm;
			dataToSend[1] = hr_bpm;
			printk("[NOTIFICATION] Heart Rate %u bpm\n", hr_bpm);
			data_service_send(peripheralConn,dataToSend,sizeof(dataToSend));
		}
	} else 
	{
		printk("[NOTIFICATION] data %p length %u\n", data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

bool deviceManager::checkAddresses(char addr1[],char addr2[])
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
