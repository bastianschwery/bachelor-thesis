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

package no.nordicsemi.android.csc.adapter;

import android.bluetooth.BluetoothDevice;
import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import no.nordicsemi.android.support.v18.scanner.ScanResult;

public class DiscoveredBluetoothDevice implements Parcelable {
	private final BluetoothDevice device;
	private ScanResult lastScanResult;
	private String name;
	private int rssi;
	private int previousRssi;
	private int highestRssi = -128;

	/**
	 * public constructor
	 * @param scanResult result of scanning
	 */
	public DiscoveredBluetoothDevice(@NonNull final ScanResult scanResult) {
		device = scanResult.getDevice();
		update(scanResult);
	}

	/**
	 * getter
	 * @return discovered device
	 */
	@NonNull
	public BluetoothDevice getDevice() {
		return device;
	}

	/**
	 * getter
	 * @return address of discovered device
	 */
	@NonNull
	public String getAddress() {
		return device.getAddress();
	}

	/**
	 * getter
	 * @return name of discovered device
	 */
	@Nullable
	public String getName() {
		return name;
	}

	/**
	 * getter
	 * @return rssi value of discovered device
	 */
	@SuppressWarnings("WeakerAccess")
	public int getRssi() {
		return rssi;
	}

	/**
	 * getter
	 * @return last scan result
	 */
	@NonNull
	public ScanResult getScanResult() {
		return lastScanResult;
	}
	/**
	 * Returns the highest recorded RSSI value during the scan.
	 *
	 * @return Highest RSSI value.
	 */
	public int getHighestRssi() {
		return highestRssi;
	}

	/**
	 * This method returns true if the RSSI range has changed. The RSSI range depends on drawable
	 * levels from {@link no.nordicsemi.android.csc.R.drawable#ic_signal_bar}.
	 *
	 * @return True, if the RSSI range has changed.
	 */
	/* package */ boolean hasRssiLevelChanged() {
		final int newLevel =
				rssi <= 10 ?
						0 :
						rssi <= 28 ?
								1 :
								rssi <= 45 ?
										2 :
										rssi <= 65 ?
												3 :
												4;
		final int oldLevel =
				previousRssi <= 10 ?
						0 :
						previousRssi <= 28 ?
								1 :
								previousRssi <= 45 ?
										2 :
										previousRssi <= 65 ?
												3 :
												4;
		return newLevel != oldLevel;
	}

	/**
	 * Updates the device values based on the scan result.
	 *
	 * @param scanResult the new received scan result.
	 */
	public void update(@NonNull final ScanResult scanResult) {
		lastScanResult = scanResult;
		name = scanResult.getScanRecord() != null ?
				scanResult.getScanRecord().getDeviceName() : null;
		previousRssi = rssi;
		rssi = scanResult.getRssi();
		if (highestRssi < rssi)
			highestRssi = rssi;
	}

	/**
	 * compare two device addresses
	 * @param scanResult result of scanning
	 * @return boolean if scan result has the same address as the discovered device
	 */
	public boolean matches(@NonNull final ScanResult scanResult) {
		return device.getAddress().equals(scanResult.getDevice().getAddress());
	}

	/**
	 * getter
	 * @return hash code of discovered device
	 */
	@Override
	public int hashCode() {
		return device.hashCode();
	}

	/**
	 * compare two device addresses
	 * @param o object
	 * @return boolean if object has same address as discovered device
	 */
	@Override
	public boolean equals(final Object o) {
		if (o instanceof DiscoveredBluetoothDevice) {
			final DiscoveredBluetoothDevice that = (DiscoveredBluetoothDevice) o;
			return device.getAddress().equals(that.device.getAddress());
		}
		return super.equals(o);
	}

	// Parcelable implementation

	/**
	 * private constructor
	 * @param in parcel
	 */
	private DiscoveredBluetoothDevice(final Parcel in) {
		device = in.readParcelable(BluetoothDevice.class.getClassLoader());
		lastScanResult = in.readParcelable(ScanResult.class.getClassLoader());
		name = in.readString();
		rssi = in.readInt();
		previousRssi = in.readInt();
		highestRssi = in.readInt();
	}

	/**
	 * write to parcel
	 * @param parcel object
	 * @param flags integer
	 */
	@Override
	public void writeToParcel(final Parcel parcel, final int flags) {
		parcel.writeParcelable(device, flags);
		parcel.writeParcelable(lastScanResult, flags);
		parcel.writeString(name);
		parcel.writeInt(rssi);
		parcel.writeInt(previousRssi);
		parcel.writeInt(highestRssi);
	}

	/**
	 *
	 * @return 0
	 */
	@Override
	public int describeContents() {
		return 0;
	}

	public static final Creator<DiscoveredBluetoothDevice> CREATOR = new Creator<DiscoveredBluetoothDevice>() {
		/**
		 * create new device with a parcel
		 * @param source parcel object
		 * @return new device
		 */
		@Override
		public DiscoveredBluetoothDevice createFromParcel(final Parcel source) {
			return new DiscoveredBluetoothDevice(source);
		}

		/**
		 * makes new array
		 * @param size integer size of array
		 * @return empty array with the given size
		 */
		@Override
		public DiscoveredBluetoothDevice[] newArray(final int size) {
			return new DiscoveredBluetoothDevice[size];
		}
	};
}
