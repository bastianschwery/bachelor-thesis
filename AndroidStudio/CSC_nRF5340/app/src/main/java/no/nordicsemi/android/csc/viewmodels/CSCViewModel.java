/*
 * Copyright (c) 2018, Nordic Semiconductor
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package no.nordicsemi.android.csc.viewmodels;

import android.app.Application;
import android.bluetooth.BluetoothDevice;

import androidx.annotation.NonNull;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;

import no.nordicsemi.android.ble.livedata.state.ConnectionState;
import no.nordicsemi.android.csc.adapter.DiscoveredBluetoothDevice;
import no.nordicsemi.android.csc.profile.CSCManager;
import no.nordicsemi.android.log.LogSession;
import no.nordicsemi.android.log.Logger;

public class CSCViewModel extends AndroidViewModel {
	private final CSCManager CSCManager;
	private BluetoothDevice device;

	/**
	 * constructor
	 * @param application object
	 */
	public CSCViewModel(@NonNull final Application application) {
		super(application);

		// Initialize the manager.
		CSCManager = new CSCManager(getApplication());
	}

	/**
	 * get the connection state
	 * @return connection state
	 */
	public LiveData<ConnectionState> getConnectionState() {
		return CSCManager.getState();
	}

	/**
	 * get the rpm value from the CSCManager
	 * @return the rpm value
	 */
	public LiveData<Integer> getRPMValue() { return CSCManager.getRPMValue();}

	/**
	 * get the speed value from the CSCManager
	 * @return the speed value
	 */
	public LiveData<Double> getSpeedValue() { return CSCManager.getSpeedValue();}

	/**
	 * get the heart rate value from the CSCManager
	 * @return the heart rate value
	 */
	public LiveData<Integer> getHeartRateValue() { return CSCManager.getHeartRateValue();}

	/**
	 * get the battery level of the first sensor from the CSCManager
	 * @return the battery level
	 */
	public LiveData<Integer> getBatteryLevelSpeed() { return CSCManager.getBatteryLevelSpeed();}

	/**
	 * get the battery level of the first sensor from the CSCManager
	 * @return the battery level
	 */
	public LiveData<Integer> getBatteryLevelCadence() { return CSCManager.getBatteryLevelCadence();}

	/**
	 * get the battery level of the first sensor from the CSCManager
	 * @return the battery level
	 */
	public LiveData<Integer> getBatteryLevelHeartRate() { return CSCManager.getBatteryLevelHeartRate();}

	/**
	 * get error code
	 * @return error code
	 */
	public LiveData<Integer> getMessageCode() { return  CSCManager.getMessageCode();}

	/**
	 * Connect to the given peripheral.
	 * @param target the target device.
	 */
	public void connect(@NonNull final DiscoveredBluetoothDevice target) {
		// Prevent from calling again when called again (screen orientation changed).
		if (device == null) {
			device = target.getDevice();
			final LogSession logSession = Logger
					.newSession(getApplication(), null, target.getAddress(), target.getName());
			CSCManager.setLogger(logSession);
			reconnect();
		}
	}

	/**
	 * Reconnects to previously connected device.
	 * If this device was not supported, its services were cleared on disconnection, so
	 * reconnection may help.
	 */
	public void reconnect() {
		if (device != null) {
			CSCManager.connect(device)
					.retry(3, 100)
					.useAutoConnect(false)
					.enqueue();
		}
	}

	/**
	 * Disconnect from peripheral.
	 */
	private void disconnect() {
		device = null;
		CSCManager.disconnect().enqueue();
	}

	/**
	 * sends a command to set the wheel diameter to value
	 * @param value diameter of the wheels in inch
	 */
	public void sendWheelDiameter(final Integer value) {
		CSCManager.sendDiameter(value);}

	/**
	 * sends a command to set notifications manually to the CSCManager
	 */
	public void setNotifications() {
		CSCManager.setNotificationsOn();}

	/**
	 * sends command to reset the diameter value to the CSCManager
	 */
	public void resetDiameter() {
		CSCManager.resetDiameterValue();}

	/**
	 * sends command to send addresses to the CSCManager
 	 * @param addresses
	 */
	public void sendAddresses(byte[] addresses) {
		CSCManager.sendAddresses(addresses);
	}

	/**
	 * when connected -> disconnect
	 */
	@Override
	protected void onCleared() {
		super.onCleared();
		if (CSCManager.isConnected()) {
			disconnect();
		}
	}

	public void resetValues() {
		device = null;
	}
}
