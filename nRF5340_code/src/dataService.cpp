
#include "DataService.h"

/*---------------------------------------------------------------------------
 * GLOBAL VARIABLES
 *--------------------------------------------------------------------------*/ 
uint8_t cntAddresses = 0;
uint8_t diameter;
double dia;
uint8_t nbrAddresses = 0;
char address1[17];
char address2[17];
char address3[17];
uint8_t infoSensors = 0;
bool notificationsOn = false;

// data arrays
uint8_t data_rx[MAX_TRANSMIT_SIZE];
uint8_t data_tx[MAX_TRANSMIT_SIZE];

// must be called befor sending/receiving data
uint8_t data_service_init(void)
{
    uint8_t err = 0;

    memset(&data_rx, 0, MAX_TRANSMIT_SIZE);
    memset(&data_tx, 0, MAX_TRANSMIT_SIZE);

    return err;
}

// This function is called whenever the RX Characteristic has been written to by a Client 
static ssize_t on_receive(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
    const uint8_t * buffer = (uint8_t *) buf;
    
    // len = 1 -> new diameter received - or diameter reset (when 0)
    if (len == 1)
    {
        diameter = (uint8_t ) *buffer;

        // check if last bit is '1', then add 0.5 to dia and convert it to cm
        if ((diameter & 0b10000000) == 0b10000000)
        {
            dia = (diameter + 0.5) * 2.54;
        }
        else 
        {
            dia = diameter * 2.54;
        }
    }   
    
    // len = 19 -> addresses of one or more sensors to connect, received
    // bits 0-17 address, bit 18 nbr of total addresses, bit 19 info about which sensors to connect
    if (len == 19)
    {
        nbrAddresses = (uint8_t) buffer[17];
        infoSensors = (uint8_t) buffer[18];

        switch (nbrAddresses)
        {
        case 1:
            for (uint8_t i=0; i<17; i++)
            {
                uint8_t val = (uint8_t) buffer[i];
                char charToSave = (char) val;
                address1[i] = charToSave;
            } 
            break;
        case 2:
            cntAddresses++;
            if (cntAddresses == 1)
            {
                for (uint8_t i=0; i<17; i++)
                {
                    uint8_t val = (uint8_t) buffer[i];
                    char charToSave = (char) val;
                    address1[i] = charToSave;
                } 
            }
            else if (cntAddresses == 2)
            {
                for (uint8_t i=0; i<17; i++)
                {
                    uint8_t val = (uint8_t) buffer[i];
                    char charToSave = (char) val;
                    address2[i] = charToSave;
                } 
                cntAddresses = 0;
            }
            break;
        case 3:
            cntAddresses++;
            if (cntAddresses == 1)
            {
                for (uint8_t i=0; i<17; i++)
                {
                    uint8_t val = (uint8_t) buffer[i];
                    char charToSave = (char) val;
                    address1[i] = charToSave;
                } 
            }
            else if (cntAddresses == 2)
            {
                for (uint8_t i=0; i<17; i++)
                {
                    uint8_t val = (uint8_t) buffer[i];
                    char charToSave = (char) val;
                    address2[i] = charToSave;
                } 
            } else if (cntAddresses == 3)
            {
                for (uint8_t i=0; i<17; i++)
                {
                    uint8_t val = (uint8_t) buffer[i];
                    char charToSave = (char) val;
                    address3[i] = charToSave;
                } 
                cntAddresses = 0;
            }     
            break;
        default:
            break;
        }
    }
    
	printk("Received data, handle %d, conn %p, data: 0x", attr->handle, conn);
 
    for(uint8_t i = 0; i < len; i++)
    {
        printk("%02X", buffer[i]);
    }
    printk("\n");
 	return len;
}

// This function is called whenever a notification has been sent by the TX Characteristic 
static void on_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

    const bt_addr_le_t * addr = bt_conn_get_dst(conn);
    printk("Data sent to Address 0x %02X %02X %02X %02X %02X %02X \n", addr->a.val[0]
                                                                , addr->a.val[1]
                                                                , addr->a.val[2]
                                                                , addr->a.val[3]
                                                                , addr->a.val[4]
                                                                , addr->a.val[5]);
}

// This function is called whenever the CCCD register has been changed by the client
void on_cccd_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    ARG_UNUSED(attr);

    switch(value)
    {
        case BT_GATT_CCC_NOTIFY: 
            // Start sending stuff!
            printk("Notifications ON\n");
            notificationsOn = true;
            break;

        case BT_GATT_CCC_INDICATE: 
            // Start sending stuff via indications
            printk("Notifications ON with Indications\n");
            notificationsOn = true;
            break;

        case 0: 
            // Stop sending stuff
            printk("Notifications OFF\n");
            notificationsOn = false;
            break;
        
        default: 
            printk("Error, CCCD has been set to an invalid value\n");     
    }
}
                        
// Data service declaration and registration 
BT_GATT_SERVICE_DEFINE(data_service,
BT_GATT_PRIMARY_SERVICE(BT_UUID_DATA_SERVICE),
BT_GATT_CHARACTERISTIC(BT_UUID_DATA_SERVICE_RX,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, 
                   NULL, on_receive, NULL),
BT_GATT_CHARACTERISTIC(BT_UUID_DATA_SERVICE_TX,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
                   NULL, NULL, NULL),
BT_GATT_CCC(on_cccd_changed,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* This function sends a notification to a Client with the provided data,
 * given that the Client Characteristic Control Descripter has been set to Notify (0x1).
 * It also calls the on_sent() callback if successful
 */
void data_service_send(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
    /* 
     * The attribute for the TX characteristic is used with bt_gatt_is_subscribed 
     * to check whether notification has been enabled by the peer or not.
     * Attribute table: 0 = Service, 1 = Primary service, 2 = RX, 3 = TX, 4 = CCC.
     */
    const struct bt_gatt_attr *attr = &data_service.attrs[3]; 

    struct bt_gatt_notify_params params = 
    {
        .uuid   = BT_UUID_DATA_SERVICE_TX,
        .attr   = attr,
        .data   = data,
        .len    = len,
        .func   = on_sent
    };

    // Check whether notifications are enabled or not
    if(bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) 
    {
        // Send the notification
	    if(bt_gatt_notify_cb(conn, &params))
        {
            printk("Error, unable to send notification\n");
        }
    }
    else
    {
        printk("Warning, notification not enabled on the selected attribute\n");
    }
}

double getDiameter() 
{
    return dia;
}

void setDiameter(uint8_t diameter) 
{
    dia = diameter;
}

uint8_t getNbrOfAddresses() 
{
    return nbrAddresses;
}

void getAddress(char* outArray, uint8_t nbr) 
{
    switch (nbr)
    {
    case 1:
        for (uint8_t i=0; i<17; i++)
        {
            outArray[i] = address1[i];
        }    
        break;
    case 2:
        for (uint8_t i=0; i<17; i++)
        {
            outArray[i] = address2[i];
        }    
        break;
    case 3:
        for (uint8_t i=0; i<17; i++)
        {
            outArray[i] = address3[i];
        }    
        break;
    default:
        break;
    }
}

uint8_t getSensorInfos() 
{
    return infoSensors;
}

bool areNotificationsOn()
{
    return notificationsOn;
}