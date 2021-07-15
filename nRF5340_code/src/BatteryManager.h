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


#define KEY_READVAL_MASK DK_BTN1_MSK

#define BAS_READ_VALUE_INTERVAL (10 * MSEC_PER_SEC)

class BatteryManager {
public:
    BatteryManager();
    static void notify_battery_level_cb(struct bt_bas_client *bas, uint8_t battery_level);

    static void discovery_completed_cb(struct bt_gatt_dm *dm, void *context);

    static void discovery_service_not_found_cb(struct bt_conn *conn, void *context);

    static void discovery_error_found_cb(struct bt_conn *conn, int err, void *context);

    static void gatt_discover(struct bt_conn *conn);

    static void read_battery_level_cb(struct bt_bas_client *bas, uint8_t battery_level, int err);

    static void button_readval(void);

    static void button_handler(uint32_t button_state, uint32_t has_changed);

    static uint8_t getBatteryLevel();

    static void initBatteryManager();


protected:
    static struct bt_bas_client bas;
    //static uint8_t batteryLevel;
};