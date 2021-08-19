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

package no.nordicsemi.android.csc.profile.callback;

import android.bluetooth.BluetoothDevice;
import androidx.annotation.NonNull;

import org.jetbrains.annotations.NotNull;

import no.nordicsemi.android.ble.callback.profile.ProfileDataCallback;
import no.nordicsemi.android.ble.data.Data;

public abstract class TXDataCallback implements ProfileDataCallback, TXCallback {
    /**
     * called when new data received
     * @param device the target device
     * @param data the new data
     */
    @Override
    public void onDataReceived(@NonNull final BluetoothDevice device, @NonNull final Data data) {
        if (data.size() > 4) {
            onInvalidDataReceived(device, data);
            return;
        }

        // message code received
        if (data.size() == 1) {
            Integer[] messageArray = new Integer[1];
            messageArray[0] = data.getIntValue(Data.FORMAT_UINT8,0);
            onCSCDataChanged(device,messageArray);
        }

        // heart rate value received
        if (data.size() == 2) {
            // 1. value: type -> heart rate
            // 2. value: heart rate value
            Integer[] dataArray = new Integer[2];
            Integer type = data.getIntValue(Data.FORMAT_UINT8,0);
            Integer val = data.getIntValue(Data.FORMAT_UINT8,1);
            dataArray[0] = type;
            dataArray[1] = val;

            onCSCDataChanged(device,dataArray);
        }

        // CSC value received
        if (data.size() == 3) {
            // 1. value: type -> 1 = speed, 2 = cadence
            // 2. value: when speed -> 8 bit of speed on the left side of the comma
            //           when cadence -> 8 lsb of cadence value
            // 3. value: when speed -> 8 bit of speed on the right side of the comma
            //           when cadence -> 8 msb of cadence value
            Integer[] dataArray = new Integer[3];
            Integer type = data.getIntValue(Data.FORMAT_UINT8,0);
            Integer val1 = data.getIntValue(Data.FORMAT_UINT8,1);
            Integer val2 = data.getIntValue(Data.FORMAT_UINT8,2);
            dataArray[0] = type;
            dataArray[1] = val1;
            dataArray[2] = val2;

            onCSCDataChanged(device,dataArray);
        }

        // battery level received
        if (data.size() == 4) {
            // 1. value: type -> battery
            // 2. value: which sensors battery level
            // 3. value: the battery level
            Integer[] dataArray = new Integer[3];
            Integer type = data.getIntValue(Data.FORMAT_UINT8,0);
            Integer sensorType = data.getIntValue(Data.FORMAT_UINT8,1);
            Integer batteryLevel = data.getIntValue(Data.FORMAT_UINT8,2);
            dataArray[0] = type;
            dataArray[1] = sensorType;
            dataArray[2] = batteryLevel;

            onCSCDataChanged(device,dataArray);
        }
    }

    /**
     * abstract function
     * @param device the target device
     * @param data first value in array is type of sensor, second value is the speed/cadence
     */
    public abstract void onCSCDataChanged(@NonNull @NotNull BluetoothDevice device, Integer[] data);
}
