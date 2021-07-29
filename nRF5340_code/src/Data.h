/**
 * @file Data.h
 * @author Schwery Bastian (bastian98@gmx.ch)
 * @brief This class saves all the received 
 *        data from the nRF5340 and calculates
 *        the speed and the cadence
 * @version 0.1
 * @date 2021-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <stdint.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

/*---------------------------------------------------------------------------
 * DEFINES
 *--------------------------------------------------------------------------*/ 
#define CSC_SPEED 1
#define CSC_CADENCE 2
#define PI 3.1415926

class Data {
public:
    /**
     * @brief constructor
     * 
     */
    Data();

    /** @brief save the new received data
     * 
     * @param data the new received data pointer 
     */
    void saveData(const void *data);

    /** @brief function to calculate rpm with the previously saved values
     * 
     * @return rounds per minute value
     */
    uint16_t calcRPM();

    /** @brief function to calculate speed with the previously saved values
     * 
     * @return speed in km/h
     */
    uint16_t calcSpeed();

    // received data from the CSC sensors
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
    double wheelDiameter;

    // received data from heart rate sensor
    uint8_t heartRate;

    // received battery values from the sensors
    uint8_t battValue_speed;
    uint8_t battValue_cadence;
    uint8_t battValue_heartRate;

    // attributes for calculations
private:
    double speed;
    double rpm;
};