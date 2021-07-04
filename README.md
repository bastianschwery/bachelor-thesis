# bachelor-thesis 2021

This is my github repository of my bachelor-thesis. 

### Description:
Nordic Semiconductor recently stated that further development of the traditional Bluetooth low Energy Stack, the so called Softdevice, will be discontinued and versions will be frozen. All new development will be done on ZEPHYR, a recent and very powerful RTOS. At the same time, Nordic has released a new Chip family, the NRF53xxx. This are dual core devices with one core being occupied with the Radio and one with the user software. The goal of this bachelor work is to develop a simple BT5 demonstrator using a NRF53 and Zephyr. The demonstrator is a light electrical vehicle (LEV) board computer: This means that it can be detected by a smartphone exposing a service similar to the ANT+ LEV service. But it must also be able to detect a number of sensors / actuators installed on the vehicle like cadence sensors, speed sensors and so on. This means that it's Bluetooth stack has to be a dual stack and that it can play at the same time the role of a central and a peripheral Bluetooth device.

### Objectives
- Develop a Zephyr based central device able to use the CSC profile.
- Develop a Zephyr based peripheral device exposing a service similar to the ANT+ LEV service.
- Develop a simple Android application in order to monitor the LEV service
- Merge the central and the peripheral device in order to obtain a hybrid device
- Establish a technical documentation.

### Content:
It contains 3 parts. 
 
 1. The code for the nRF5340, for the peripheral and central connection (nRF_5340_code folder)
 2. The code for the android application (android_studio folder)
 3. The report with all annexes (docs folder)
