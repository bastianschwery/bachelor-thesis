/**
 * @author  Schwery Bastian
 * @file    dataCSC.h   
 * @date    05/2021
 * 
 * @brief   This class saves all the received 
 *          data from the nRF5340 and calculates
 *          the speed and the cadence
 */

#include <stdint.h>

#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

// defines
#define _USE_MATH_DEFINES
#define PI 3.1415926
#define CSC_SPEED 1
#define CSC_CADENCE 2

class dataCSC {

public:
    // constructor
    dataCSC();

    /** @brief save the new received data
     * @param data the new received data pointer 
     */
    void saveData(const void *data);

    /** @brief function to calculate rpm with the previously saved values
     * @return rounds per minute value
     */
    uint16_t calcRPM();

    /** @brief function to calculate speed with the previously saved values
     * @return speed in km/h
     */
    uint16_t calcSpeed();

    // received data attributes from the CSC sensors
    uint16_t sumRevSpeed;
    uint16_t oldSumRevSpeed;
    uint16_t sumRevCadence;
    uint16_t oldSumRevCadence;
    uint16_t lastEventSpeed;
    uint16_t oldLastEventSpeed;
    uint16_t lastEventCadence;
    uint16_t oldLastEventCadence;
    uint8_t type;

    // received data from application
    uint8_t wheelDiameter;

    // attributes for calculations
private:
    double speed;
    double rpm;
};