/*
 * Author: Schwery Bastian
 * Date: 05/2021
 * Cours: Bachelor Thesis
 */

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

// CSC type definitions
#define CSC_SPEED 1
#define CSC_CADENCE 2

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

static struct devices{
    int CENTRAL;
    int PERIPHERAL;
    int CENTRAL_PERIPHERAL;
}device;

// initalize static attributes
bool deviceManager::isCentral = false;
bool deviceManager::isPeripheral = false;
bool deviceManager::connectedC = false;
bool deviceManager::connectedP = false;
bool deviceManager::app_button_state = false;
bool deviceManager::subscriptionDone = false;
bool deviceManager::diameterSet = false;
bool deviceManager::once_speed = false;
bool deviceManager::once_cadence = false;

uint8_t deviceManager::nbrConnectionsCentral = 0;
bt_conn* deviceManager::peripheralConn;
bt_conn* deviceManager::centralConnections[];
dataCSC deviceManager::data;

/*-----------------------------------------------------------------------------------------------------
 * GENERAL METHODS
 *---------------------------------------------------------------------------------------------------*/

/*
 * constructor
 */
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

/*
 * getter
 */
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

/*
 * setter
 */
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

/*
 * led callback method
 */
void deviceManager::app_led_cb(bool led_state){
	// set led to led_state
    dk_set_led(USER_LED,led_state);
}

/*
 * button callback method
 */
bool deviceManager::app_button_cb(void){
    return app_button_state;
}

/*
 * callback method
 * called when user button has been pressed or released
 */
void deviceManager::buttonChanged(uint32_t button_state, uint32_t has_changed){
   	if (has_changed & USER_BUTTON) {
		bt_lbs_send_button_state(button_state);
		app_button_state = button_state ? true : false;
	} 
}

/*
 * initialize button service
 */
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

/*
 * initialize all necessary settings to use the board in peripheral role 
 */
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

/*
 * starts advertising and waites to be connected to a central device
 */
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
 
 /*
 * initialize all necessary settings to use the board in central role 
 */
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

/*
 * initialize scan 
 */
void deviceManager::initScan() {
	int err;

	BT_SCAN_CB_INIT(scan_cb, scanFilterMatch, NULL, scanConnectionError, NULL);

	struct bt_le_scan_param scanParam =
        {
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

	bt_scan_init(&scanInit);
	bt_scan_cb_register(&scan_cb);

	// add CSC UUID filter
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_CSC);
	if (err) {
		printk("Scanning filters cannot be set\n");
		return;
	}

	// enable filters
	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on\n");
	}
}

/*
 * start scanning for peripheral devices
 * when device found call callback method deviceFound
 */
void deviceManager::startScan(){
    int err;
	
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start, err %d\n", err);
	}
	printk("Scanning...\n");
}

/*
 * callback function
 * found a device with the added filters
 */
void deviceManager::scanFilterMatch(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable) {
    char speed_sensor_1[18] = "D4:D6:5E:D1:66:D";
	char speed_sensor_2[18] = "D9:3F:F2:D1:0B:1B";
	char cadence_sensor_1[18] = "C4:64:9B:C6:7B:AE";
	char cadence_sensor_2[18] = "E6:6C:AF:76:18:AD";

    char addr[BT_ADDR_LE_STR_LEN];
	uint8_t err;

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %s\n",
		addr, connectable ? "yes" : "no");

	if ((strstr(addr,speed_sensor_1) || strstr(addr,speed_sensor_1)) && !once_speed)
	{
		once_speed = true;
		bt_scan_stop();
		err = bt_conn_le_create(device_info->recv_info->addr,
								BT_CONN_LE_CREATE_CONN,
								device_info->conn_param, &centralConnections[nbrConnectionsCentral]);
	}

	if ((strstr(addr,cadence_sensor_1) || strstr(addr,cadence_sensor_2)) && !once_cadence)
	{
		once_cadence = true;
		bt_scan_stop();
		err = bt_conn_le_create(device_info->recv_info->addr,
								BT_CONN_LE_CREATE_CONN,
								device_info->conn_param, &centralConnections[nbrConnectionsCentral]);
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

/*
 * callback method
 * called if a device was found during scanning
 * try to connect to this device when its the correct one
 * used for etablish connection with a thingy
 */
void deviceManager::deviceFound(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad){
	char addr_str[BT_ADDR_LE_STR_LEN];
	char addr_toConnect[18] = "EA:1A:AD:15:54:9F";
	int err = 0;
	static int cnt = 0;

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	/* connect only to devices in close proximity */
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
    }   
}

/*
 * callback method
 * called when nRF5340 connects to a central or peripheral device
 */
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
		
		// discovery callback
		static struct bt_gatt_dm_cb discovery_cb = 
		{
			.completed = deviceManager::discoveryCompleted,
			.service_not_found = deviceManager::discovery_service_not_found,
			.error_found = deviceManager::discovery_error_found,
		};

		// save connection 
		centralConnections[nbrConnectionsCentral] = conn;
		nbrConnectionsCentral++;
		err = bt_gatt_dm_start(centralConnections[nbrConnectionsCentral-1], BT_UUID_CSC, &discovery_cb, NULL);
		if (err) 
		{
			printk("Could not start service discovery, err %d\n", err);
		}

	}
	else if (info.role == BT_CONN_ROLE_SLAVE)	// slave -> peripheral role
	{
		if (err) {
			printk("Connection failed (err %u)\n", err);
			return;
		}
		printk("Connected\n");
		connectedP = true;
		peripheralConn = conn;
		dk_set_led_on(CON_STATUS_LED_PERIPHERAL);	
		

		// when its in central and peripheral mode -> begin scanning
		if (getDevice() == 3 && nbrConnectionsCentral == 0) 
		{
			initScan();
			startScan();
		}
	}	
}

/*
 * callback method
 * called when board disconnects from a central or peripheral device
 */
void deviceManager::disconnected(struct bt_conn *conn, uint8_t reason) {
	bt_conn_info info;
	int error = bt_conn_get_info(conn,&info);
	char speed_sensor_1[18] = "D4:D6:5E:D1:66:DB";
	char speed_sensor_2[18] = "D9:3F:F2:D1:0B:1B";
	char cadence_sensor_1[18] = "C4:64:9B:C6:7B:AE";
	char cadence_sensor_2[18] = "E6:6C:AF:76:18:AD";
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
		peripheralConn = nullptr;
		dk_set_led_off(CON_STATUS_LED_PERIPHERAL);
		startAdvertising();
	}
	else if (info.role == BT_CONN_ROLE_MASTER)	// master -> central role
	{
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		printk("Disconnected from Sensor: %s (reason 0x%02x)\n", addr, reason);

		// delete the correct connection in the array
		for (uint8_t i = 0; i < nbrConnectionsCentral-1; i++)
		{
			if (centralConnections[i] == conn)
			{
				centralConnections[i] = nullptr;
				nbrConnectionsCentral--;
				disconnectedCode[0] = 11;
				data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
				return;
			}	
		}

		if (nbrConnectionsCentral == 0)
		{
			connectedC = false;
			disconnectedCode[0] = 12;
			data_service_send(peripheralConn,disconnectedCode, sizeof(disconnectedCode));
			dk_set_led_off(CON_STATUS_LED_CENTRAL);
		}
		// start scanning again

		if (strstr(addr,speed_sensor_1) || strstr(addr,speed_sensor_2))
		{
			once_speed = false;
		}

		if (strstr(addr,cadence_sensor_1) || strstr(addr,cadence_sensor_2))
		{
			once_cadence = false;
		}
		
		startScan();	
	}
}

/*
 * callback method
 * called when discovery is finished
 */
void deviceManager::discoveryCompleted(struct bt_gatt_dm *disc, void *ctx) {
	int err;
	uint8_t connectedCode[1];
	// subscribe button characteristic
	static struct bt_gatt_subscribe_params param = {
		.notify = onReceived,
		.value = BT_GATT_CCC_NOTIFY,
	};

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;

	// Get the characteristic by its UUID
	chrc = bt_gatt_dm_char_by_uuid(disc,BT_UUID_CSC_MEASUREMENT);
	if (!chrc) {
		printk("Missing CSC measurement characteristic\n");
		err = bt_gatt_dm_data_release(disc);
		if (err) {
			printk("Could not release discovery data, err: %d\n", err);
		}
		return;
	}

	// Search the descriptor by its UUID
	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_CSC_MEASUREMENT);
	if (!desc) {
		printk("Missing CSC measurement char CCC descriptor\n");
		err = bt_gatt_dm_data_release(disc);
		if (err) {
			printk("Could not release discovery data, err: %d\n", err);
		}
		return;
	}

	param.value_handle = desc->handle;

	// Search the CCC descriptor by its UUID
	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		printk("Missing CSC measurement char CCC descriptor\n");
		err = bt_gatt_dm_data_release(disc);
		if (err) {
			printk("Could not release discovery data, err: %d\n", err);
		}
		return;
	}

	param.ccc_handle = desc->handle;
	
	// Subscribe Attribute Value Notification
	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
	if (err) {
		printk("Subscribtion failed (err %d)\n", err);
	}
	bt_gatt_dm_data_release(disc);

	// check number of connections -> can be modified for more devices
	switch (nbrConnectionsCentral)
	{
	case 1:
		connectedCode[0] = 13;
		data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
		startScan();
		printk("First discovery completed\n");
		break;
	case 2:
		connectedCode[0] = 14;
		data_service_send(peripheralConn,connectedCode, sizeof(connectedCode));
		printk("Second discovery completed\n");
		dk_set_led_on(CON_STATUS_LED_CENTRAL);
		connectedC = true;	
		subscriptionDone = true;
		break;
	default:
		break;
	}
}

/*
 * callback method
 * called when discovery service was not found
 */
void deviceManager::discovery_service_not_found(struct bt_conn *conn, void *ctx) {
	printk("Service not found!\n");
	static uint8_t cnt = 0;
	uint8_t err;
	uint8_t error[1];
	error[0] = 10;
	data_service_send(peripheralConn,error, sizeof(error));
	//bt_conn_disconnect(conn,-5);

	// discovery callback
	static struct bt_gatt_dm_cb discovery_cb = 
	{
		.completed = deviceManager::discoveryCompleted,
		.service_not_found = deviceManager::discovery_service_not_found,
		.error_found = deviceManager::discovery_error_found,
	};

	if (cnt < 2)
	{
		cnt++;
		printk("nbr central connections: %d\n", nbrConnectionsCentral);
		err = bt_gatt_dm_start(conn, BT_UUID_CSC, &discovery_cb, NULL);
		if (err) 
		{
			printk("Could not start service discovery, err %d\n", err);
		}
	}
	else 
	{
		cnt = 0;
		printk("Service definitly not found -> Restart application\n");
	}	
}

/*
 * callback method
 * called when while discovering an error appears
 */
void deviceManager::discovery_error_found(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}

 /*
 * callback method
 * called every second with data
 */
uint8_t deviceManager::onReceived(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length) {

	// start calculating and showing data only when all characteristics are subscribed
	if (subscriptionDone)
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
			
			if (deviceManager::data.type == CSC_SPEED)
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
						dataToSend[0] = CSC_SPEED;
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
			}
			else if (deviceManager::data.type == CSC_CADENCE)
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
						dataToSend[0] = CSC_CADENCE;	
						dataToSend[1] = (uint8_t) rpm;
						dataToSend[2] = (uint8_t) (rpm >> 8);	
						if (peripheralConn != nullptr)
						{
							printk("Cadence rpm: %d\n",rpm);
							data_service_send(peripheralConn,dataToSend, sizeof(dataToSend));
						}
					}
				}
			}
		}
	}
	return BT_GATT_ITER_CONTINUE;
}
