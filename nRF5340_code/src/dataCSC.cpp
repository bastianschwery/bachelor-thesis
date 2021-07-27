#include "dataCSC.h"

dataCSC::dataCSC() 
{
    sumRevSpeed = 0;
    oldSumRevSpeed = 0;
    sumRevCadence = 0;
    oldSumRevCadence = 0;
    lastEventSpeed = 0;
    oldLastEventSpeed = 0;
    lastEventCadence = 0;
    oldLastEventCadence = 0;
    type = 0;
    wheelDiameter = 0.0;
    heartRate = 0;
    battValue_speed = 0;
    battValue_cadence = 0;
    battValue_heartRate = 0;
    speed = 0;
    rpm = 0;
}

void dataCSC::saveData(const void *data) 
{
    type = ((uint8_t*)data)[0];
    switch (type)
    {
    case CSC_SPEED:
        // save old values
        oldSumRevSpeed = sumRevSpeed;
        oldLastEventSpeed = lastEventSpeed;

        // save new values
        sumRevSpeed = sys_get_le16(&((uint8_t*)data)[1]);
		lastEventSpeed = sys_get_le16(&((uint8_t*)data)[5]);
        break;
    case CSC_CADENCE:
        // save old values
        oldSumRevCadence = sumRevCadence;
        oldLastEventCadence  = lastEventCadence;
        // save new values
		sumRevCadence = sys_get_le16(&((uint8_t*)data)[1]);
		lastEventCadence  = sys_get_le16(&((uint8_t*)data)[3]);
        break;
    default:
        printk("Unknown type\n");
        break;
    }
}

uint16_t dataCSC::calcRPM() 
{
    uint16_t retVal = 0;
    uint16_t maxVal = 0xffff;
    uint16_t nbrRev = sumRevCadence-oldSumRevCadence;

    if (nbrRev < 0)
    {
        nbrRev = maxVal - oldSumRevCadence + sumRevCadence;
    }
    
    if ((lastEventCadence != oldLastEventCadence) && (nbrRev))
    {
        double time = (lastEventCadence - oldLastEventCadence)/1024.0;

        if (time < 0)
        {
            time = 0xffff - oldLastEventCadence + lastEventCadence;
        }
        
        rpm = (sumRevCadence - oldSumRevCadence) * 60 / time;
        retVal = (uint16_t) rpm;
    }

    return retVal;
}

uint16_t dataCSC::calcSpeed() 
{
    uint16_t retVal = 0;
    uint16_t nbrRevSpeed = sumRevSpeed - oldSumRevSpeed;
    uint32_t maxVal = 0xffffffff;   // 32 bit
    double wheelCircumference = 0;
    double rpm_speed = 0;
    
    if (nbrRevSpeed < 0)
    {
        nbrRevSpeed = maxVal - oldSumRevSpeed + sumRevSpeed;
    }

    if ((lastEventSpeed != oldLastEventSpeed) && nbrRevSpeed > 0)
    {
        wheelCircumference = wheelDiameter;
        wheelCircumference = (wheelDiameter) * PI;
        double time = (lastEventSpeed - oldLastEventSpeed)/1024.0;
        double oldSpeed = speed;

        if (time < 0)
        {
            time = (0xffff - oldLastEventSpeed + lastEventSpeed)/1024;
        }
        if (sumRevSpeed == oldSumRevSpeed)
        {
            return (uint16_t) oldSpeed;
        }
        else   
        {
            rpm_speed = (nbrRevSpeed) * 60 / time;
            speed = (rpm_speed * wheelCircumference) * 60 / 1000; // km/h * 100 for the values after the comma
            retVal = (uint16_t) (speed);
            return retVal;
        }  
    }
    return 0;
}