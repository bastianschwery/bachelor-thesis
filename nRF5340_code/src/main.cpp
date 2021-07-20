/**
 * @file 	main.cpp
 * @author 	Schwery Bastian (bastian98@gmx.ch)
 * @brief 	This class creates a deviceManager instance a
 * 			starts the application
 * @version 0.1
 * @date 	2021-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "deviceManager.h"

void main(void)
{

	printk("Application start\n");

	// create a new device manager
	deviceManager dManager;
	// start application as peripheral and central
	dManager.setDevice(true,true);
} 	