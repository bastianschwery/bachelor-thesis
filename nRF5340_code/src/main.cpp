/**
 * @author  Schwery Bastian
 * @file    main.cpp    
 * @date    05/2021
 * @reason: Bachelor Thesis
 * 
 * @brief   This class creates a deviceManager instance a
 * 			starts the application
 */

#include "deviceManager.h"

#include <bluetooth/services/bas_client.h>

//static struct bt_bas_client bas;

void main(void)
{
	printk("Application start\n");

	//bt_bas_client_init(&bas);


	// create a new device manager
	deviceManager dManager;
	// start application as peripheral and central
	dManager.setDevice(true,true);
} 	