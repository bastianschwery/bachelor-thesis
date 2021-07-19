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

package no.nordicsemi.android.csc.profile;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.jetbrains.annotations.NotNull;

import java.util.UUID;

import no.nordicsemi.android.ble.data.Data;
import no.nordicsemi.android.ble.livedata.ObservableBleManager;
import no.nordicsemi.android.csc.profile.callback.RXDataCallback;
import no.nordicsemi.android.csc.profile.callback.TXDataCallback;
import no.nordicsemi.android.log.LogContract;
import no.nordicsemi.android.log.LogSession;
import no.nordicsemi.android.log.Logger;

import static android.bluetooth.BluetoothGattCharacteristic.PROPERTY_NOTIFY;

public class CSCManager extends ObservableBleManager {
	/** Nordic Blinky Service UUID. */
	public final static UUID LBS_UUID_SERVICE = UUID.fromString("00001523-1212-efde-1523-785feabcd123");
	/** CSC data characteristic Service UUID. */
	public final static UUID CSC_SERVICE = UUID.fromString("75c276c3-8f97-20bc-a143-b354244886d4");
	/** CSC receive data characteristic UUID. */
	public final static UUID RX_CHARACTERISTIC_UUID = UUID.fromString("6ACF4F08-CC9D-D495-6B41-AA7E60C4E8A6");
	/** CSC transmit data characteristic UUID. */
	public final static UUID TX_CHARACTERISTIC_UUID = UUID.fromString("D3D46A35-4394-E9AA-5A43-E7921120AAED");

	public static final  UUID CCCD_ID = UUID.fromString("000002902-0000-1000-8000-00805f9b34fb");

	private final MutableLiveData<Integer> rpmValue = new MutableLiveData<>();
	private final MutableLiveData<Double> speedValue = new MutableLiveData<>();
	private final MutableLiveData<Integer> messageCode = new MutableLiveData<>();
	private final MutableLiveData<Integer> heartRateValue = new MutableLiveData<>();
	private final MutableLiveData<Boolean> isDisconnected = new MutableLiveData<>();

	private BluetoothGattCharacteristic RX_characteristic, TX_characteristic;

	private LogSession logSession;
	private boolean supported;

	private BluetoothGatt mBluetoothGatt;
	private double speed = 0;

	private final int TYPE_SPEED = 1;
	private final int TYPE_CADENCE= 2;
	private final int TYPE_HEARTRATE = 3;

	/**
	 * constructor
	 * @param context the context
	 */
	public CSCManager(@NonNull final Context context) {
		super(context);
	}

	/**
	 * get the rpm value
	 * @return the rpm value
	 */
	public final LiveData<Integer> getRPMValue() { return  rpmValue;}

	/**
	 * get the speed value
	 * @return the speed value
	 */
	public final LiveData<Double> getSpeedValue() { return speedValue;}

	/**
	 * get the heart rate value
	 * @return the heart rate value
	 */
	public final LiveData<Integer> getHeartRateValue() { return heartRateValue;}

	/**
	 * get the message code
	 * @return the message code
	 */
	public final LiveData<Integer> getMessageCode() { return messageCode;}

	public final LiveData<Boolean> isDisconnected() { return isDisconnected;}


	@NonNull
	@Override
	protected BleManagerGattCallback getGattCallback() {
		return new CSCBleManagerGattCallback();
	}

	/**
	 * Sets the log session to be used for low level logging.
	 * @param session the session, or null, if nRF Logger is not installed.
	 */
	public void setLogger(@Nullable final LogSession session) {
		logSession = session;
	}

	@Override
	public void log(final int priority, @NonNull final String message) {
		// The priority is a Log.X constant, while the Logger accepts it's log levels.
		Logger.log(logSession, LogContract.Log.Level.fromPriority(priority), message);
	}

	@Override
	protected boolean shouldClearCacheWhenDisconnected() {
		return !supported;
	}

	/**
	 * The CSC callback will be notified when a notification from CSC Measueremnt characteristic
	 * has been received.
	 * <p>
	 * {@link TXDataCallback#onCSCDataChanged} will be called.
	 * Otherwise, the {@link TXDataCallback#onInvalidDataReceived(BluetoothDevice, Data)}
	 * will be called with the data received.
	 */
	private final TXDataCallback txCallback = new TXDataCallback() {

		/**
		 * callback -> called when new data arrived
		 * @param device the target device
		 * @param data first value in array is type of sensor, second value is the speed/cadence/heart rate
		 */
		@Override
		public void onCSCDataChanged(@NonNull @NotNull BluetoothDevice device, Integer[] data) {
			log(Log.INFO, "Data received");

			if (data.length == 1) {
				messageCode.setValue(data[0]);
			}

			switch (data[0]) {
				case TYPE_SPEED:
					double val = data[2].doubleValue();
					speed = data[1] + val / 100;
					speedValue.setValue(speed);
					break;
				case TYPE_CADENCE:
					rpmValue.setValue(data[1] + (data[2] << 8));
					break;
				case TYPE_HEARTRATE:
					heartRateValue.setValue(data[1]);
				default:
					log(Log.INFO, "Unknown type");
					break;
			}
		}

		/**
		 * invalid data was received
		 * @param device the target device
		 * @param data the fault data
		 */
		@Override
		public void onInvalidDataReceived(@NonNull @NotNull BluetoothDevice device, @NonNull @NotNull Data data) {
			// Data can only invalid if we read them. We assume the app always sends correct data.
			log(Log.WARN, "Invalid data received: " + data);
		}
	};

	/**
	 * The RXDataCallback will be notified when a notification from RX characteristic
	 * has been received.
	 * <p>
	 * {@link RXDataCallback#onCSCDataChanged} will be called.
	 * Otherwise, the {@link RXDataCallback#onInvalidDataReceived(BluetoothDevice, Data)}
	 * will be called with the data received.
	 */
	private final RXDataCallback rxCallback = new RXDataCallback() {
		/**
		 * value changed on the rx characteristic
		 * @param device the target device
		 * @param data the new data
		 */
		@Override
		public void onCSCDataChanged(@NonNull @NotNull BluetoothDevice device, Integer data) {
			log(Log.INFO, "New Data:" + data);
		}

		/**
		 * invalid data was received
		 * @param device the target device
		 * @param data the fault data
		 */
		@Override
		public void onInvalidDataReceived(@NonNull @NotNull BluetoothDevice device, @NonNull @NotNull Data data) {
			// Data can only invalid if we read them. We assume the app always sends correct data.
			log(Log.WARN, "Invalid data received: " + data);
		}
	};

	/**
	 * BluetoothGatt callbacks object.
	 * initialize all notification settings
	 */
	private class CSCBleManagerGattCallback extends BleManagerGattCallback {
		/**
		 * initialize all notifications of the characterstics
		 */
		@Override
		protected void initialize() {
			setNotificationCallback(RX_characteristic).with(rxCallback);
			readCharacteristic(RX_characteristic).with(rxCallback).enqueue();
			readCharacteristic(TX_characteristic).with(txCallback).enqueue();
			enableNotifications(RX_characteristic).enqueue();
			setNotificationCallback(TX_characteristic).with(txCallback);
			enableNotifications(TX_characteristic).enqueue();
		}

		/**
		 *
		 * check if service is supported or not
		 * @param gatt object
		 * @return boolean supported
		 */
		@Override
		public boolean isRequiredServiceSupported(@NonNull final BluetoothGatt gatt) {
			final BluetoothGattService CSCService = gatt.getService(CSC_SERVICE);
			if (CSCService != null) {
				mBluetoothGatt = gatt;
				RX_characteristic = CSCService.getCharacteristic(RX_CHARACTERISTIC_UUID);
				TX_characteristic = CSCService.getCharacteristic(TX_CHARACTERISTIC_UUID);
			}

			boolean writeRequest = false;
			if (TX_characteristic != null) {
				final int rxProp = TX_characteristic.getProperties();
				writeRequest = (rxProp & PROPERTY_NOTIFY) > 0;
			}

			supported = RX_characteristic != null && TX_characteristic != null && writeRequest;
			return supported;
		}

		@Override
		protected void onDeviceDisconnected() {
			TX_characteristic = null;
			RX_characteristic = null;
			isDisconnected.setValue(true);
		}
	}

	/**
	 *
	 * send diameter value in inch over ble to server
	 * @param diameter value in inch
	 */
	public void sendDiameter(int diameter) {
		log(Log.VERBOSE,"Sending wheel diameter, Data: " + diameter);
		writeCharacteristic(RX_characteristic,Data.opCode((byte) diameter)).enqueue();
	}

	/**
	 * set notifications manually
	 */
	public void setNotificationsOn() {
		BluetoothGattService service = mBluetoothGatt.getService(CSC_SERVICE);
		BluetoothGattCharacteristic TXcharacteristic = service.getCharacteristic(TX_CHARACTERISTIC_UUID);
		BluetoothGattDescriptor descriptor = TXcharacteristic.getDescriptor(CCCD_ID);
		TXcharacteristic.addDescriptor(descriptor);
		descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
		TXcharacteristic.setWriteType(PROPERTY_NOTIFY);
		mBluetoothGatt.writeDescriptor(descriptor);
		mBluetoothGatt.setCharacteristicNotification(TXcharacteristic,true);
	}

	/**
	 * send message to server that the diameter has been reset
	 */
	public void resetDiameterValue() {
		writeCharacteristic(RX_characteristic,Data.opCode((byte) 0)).enqueue();
	}

	/**
	 * send addresses to connect to the server
	 * @param addresses to connect
	 */
	public void sendAddresses(byte[] addresses) {
		isDisconnected.setValue(false);
		writeCharacteristic(RX_characteristic,addresses).enqueue();
	}
}
