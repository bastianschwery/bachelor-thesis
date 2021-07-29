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

package no.nordicsemi.android.csc;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.os.Parcelable;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.appbar.MaterialToolbar;

import java.nio.ByteBuffer;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Collections;

import butterknife.BindView;
import butterknife.ButterKnife;
import butterknife.OnClick;
import no.nordicsemi.android.ble.livedata.state.ConnectionState;
import no.nordicsemi.android.csc.adapter.DiscoveredBluetoothDevice;
import no.nordicsemi.android.csc.viewmodels.CSCViewModel;

@SuppressWarnings("ConstantConditions")
public class CSCActivity extends AppCompatActivity {
	public static final String EXTRA_DEVICE = "no.nordicsemi.android.csc.EXTRA_DEVICE";
	public static final String Sensor1 = "no.nordicsemi.android.csc.SENSOR_1";

	private boolean firstEntry = false;
	private CSCViewModel viewModel;
	private TextView diameterValue;
	private Button setValueButton, resetButton, resetDistanceButton;
	private ImageView batteryIconSpeed, batteryIconCadence, batteryIconHeartRate;
	private double wheelDiameter = 0.0;
	private double distance = 0;
	private NumberFormat n1 = NumberFormat.getInstance();
	private NumberFormat n2 = NumberFormat.getInstance();
	private ArrayList<String> addresses = new ArrayList<>();
	private DiscoveredBluetoothDevice nordicBoard;
	private ArrayList<DiscoveredBluetoothDevice> devices = new ArrayList<>();
	private ArrayList<DiscoveredBluetoothDevice> sensors = new ArrayList<>();
	private Parcelable[] receivedArray = new Parcelable[10];
	byte[] address1;
	byte[] address2;
	byte[] address3;
	int nbrAddresses = 0;
	int infoDevices = 0;

	@BindView(R.id.set_button) Button button;
	@BindView(R.id.reset_button) Button rstBtn;
	@BindView(R.id.reset_distance_button) Button rstDistanceBtn;
	@BindView(R.id.speed_value) TextView speedValue;
	@BindView(R.id.cadence_value) TextView cadenceValue;
	@BindView(R.id.distance_value) TextView distanceValue;
	@BindView(R.id.heartRate_value) TextView heartRateValue;
	@BindView(R.id.battery_level_sensor1) TextView batteryLevelSpeed;
	@BindView(R.id.battery_level_sensor2) TextView batteryLevelCadence;
	@BindView(R.id.battery_level_sensor3) TextView batteryLevelHeartRate;

	public CSCActivity() {
	}

	/**
	 * create all necessary instances and add on click listeners
	 * @param savedInstanceState state
	 */
	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_csc);
		ButterKnife.bind(this);

		final Intent intent = getIntent();
		receivedArray = intent.getParcelableArrayExtra(EXTRA_DEVICE);
		for (int i=0;i<receivedArray.length;i++) {
			if (receivedArray[i] == null) {
				break;
			}
			else {
				devices.add((DiscoveredBluetoothDevice) receivedArray[i]);
				if (devices.get(i).getName().contains("Nordic")) {
					nordicBoard = devices.get(i);
				}
				else {
					sensors.add((DiscoveredBluetoothDevice) receivedArray[i]);
				}
			}
		}

		/*
		if there is just one speed -> info devices: 1
		if there is just one cadence -> info devices: 2
		if there are a speed and a cadence sensor -> info devices: 3
		if there are a speed and cadence and a heart rate sensor -> info devices: 4
		if there are a speed and a heart rate sensor -> info devices: 5
		if there are a cadence and a heart rate sensor -> info devices: 6
		if there is just a heart rate sensor -> info devices: 7

		make sure that the sequence is like: first speed/cadence sensors and at the end heart rate sensor
		 */
		infoDevices = 0;
		switch (sensors.size()) {
			case 1:
				if (sensors.get(0).getName().contains("SPD")) {
					infoDevices = 1;
				} else if (sensors.get(0).getName().contains("CAD")) {
					infoDevices = 2;
				} else if (sensors.get(0).getName().contains("Polar")) {
					infoDevices = 7;
				}
				break;
			case 2:
				if (sensors.get(0).getName().contains("SPD") || sensors.get(0).getName().contains("CAD")) {
					if (sensors.get(1).getName().contains("SPD") || sensors.get(1).getName().contains("CAD")) {
						infoDevices = 3;
					} else if (sensors.get(1).getName().contains("Polar") && sensors.get(0).getName().contains("SPD")) {
						infoDevices = 5;
					}
					else if (sensors.get(1).getName().contains("Polar") && sensors.get(0).getName().contains("CAD")) {
						infoDevices = 6;
					}
				} else if (sensors.get(0).getName().contains("Polar")) {
					if (sensors.get(1).getName().contains("SPD")) {
						infoDevices = 5;
						Collections.swap(sensors,0,1);
					} else if (sensors.get(1).getName().contains("CAD")) {
						infoDevices = 6;
						Collections.swap(sensors,0,1);
					}
				}
				break;
			case 3:
				infoDevices = 4;
				if (sensors.get(0).getName().contains("Polar")) {
					Collections.swap(sensors,0,2);
				} else if (sensors.get(1).getName().contains("Polar")) {
					Collections.swap(sensors,1,2);
				}
				break;
			default:
				break;
		}

		for (int i = 0; i < sensors.size(); i++) {
			addresses.add(sensors.get(i).getAddress());
		}

		/*
		for every sensor is a buffer
		buffer is user to add 1 byte at the end
		this 1 byte indicates how many sensor
		there are to connect
		*/
		ByteBuffer buffer1 = ByteBuffer.allocate(19);
		ByteBuffer buffer2 = ByteBuffer.allocate(19);
		ByteBuffer buffer3 = ByteBuffer.allocate(19);

		switch (addresses.size()) {
			case 1:
				// save address 1 from the pool of addresses
				address1 = addresses.get(0).getBytes();
				nbrAddresses = 1;
				// put this address in the buffer
				buffer1.put(address1);
				// put information about how many sensors to connect
				buffer1.put((byte) nbrAddresses);
				// put information which sensors to connect at the end
				buffer1.put((byte) infoDevices);
				// restore array in address 1
				address1 = buffer1.array();
				// clear buffer
				buffer1.clear();
				break;
			case 2:
				nbrAddresses = 2;
				// save address 1 and 2 from the pool of addresses
				address1 = addresses.get(0).getBytes();
				address2 = addresses.get(1).getBytes();
				// put address 1 in the buffer
				buffer1.put(address1);
				// put information byte at the end
				buffer1.put((byte) nbrAddresses);
				// put information which sensors to connect at the end
				buffer1.put((byte) infoDevices);
				// restore array in address 1
				address1 = buffer1.array();
				// clear buffer
				buffer1.clear();
				// put address 1 in the buffer
				buffer2.put(address2);
				// put information byte at the end
				buffer2.put((byte) nbrAddresses);
				// put information which sensors to connect at the end
				buffer2.put((byte) infoDevices);
				// restore array in address 2
				address2 = buffer2.array();
				// clear buffer
				buffer2.clear();
				break;
			case 3:
				nbrAddresses = 3;
				address1 = addresses.get(0).getBytes();
				address2 = addresses.get(1).getBytes();
				address3 = addresses.get(2).getBytes();
				buffer1.put(address1);
				buffer1.put((byte) nbrAddresses);
				buffer1.put((byte) infoDevices);
				address1 = buffer1.array();
				buffer1.clear();
				buffer2.put(address2);
				buffer2.put((byte) nbrAddresses);
				buffer2.put((byte) infoDevices);
				address2 = buffer2.array();
				buffer2.clear();
				buffer3.put(address3);
				buffer3.put((byte) nbrAddresses);
				buffer3.put((byte) infoDevices);
				address3 = buffer3.array();
				buffer3.clear();
				break;
			default:
				break;
		}

		final String deviceName = nordicBoard.getName();
		final String deviceAddress = nordicBoard.getAddress();

		final MaterialToolbar toolbar = findViewById(R.id.toolbar);
		toolbar.setTitle(deviceName != null ? deviceName : getString(R.string.unknown_device));
		toolbar.setSubtitle(deviceAddress);
		setSupportActionBar(toolbar);
		getSupportActionBar().setDisplayHomeAsUpEnabled(true);

		// Configure the view model.
		viewModel = new ViewModelProvider(this).get(CSCViewModel.class);
		viewModel.connect(nordicBoard);

		// Set up views.
		final LinearLayout progressContainer = findViewById(R.id.progress_container);
		final TextView connectionState = findViewById(R.id.connection_state);
		final View content = findViewById(R.id.device_container);
		final View notSupported = findViewById(R.id.not_supported);
		diameterValue = findViewById(R.id.diameter_value);
		setValueButton = findViewById(R.id.set_button);
		resetButton = findViewById(R.id.reset_button);
		resetDistanceButton = findViewById(R.id.reset_distance_button);
		batteryIconSpeed =  findViewById(R.id.batteryFull1);
		batteryIconCadence = findViewById(R.id.batteryFull2);
		batteryIconHeartRate = findViewById(R.id.batteryFull3);

		resetDistanceButton.setOnClickListener(v -> {
			distanceValue.setText("0");
			distance = 0;
		});

		resetButton.setOnClickListener(v -> {
			setValueButton.setEnabled(true);
			diameterValue.setCursorVisible(true);
			diameterValue.setText("0");
			speedValue.setText("0");
			cadenceValue.setText("0");
			viewModel.resetDiameter();
		});

		setValueButton.setOnClickListener(v -> {
			boolean valueIsValid = false;
			try {
				wheelDiameter = Double.parseDouble(diameterValue.getText().toString());
				valueIsValid = true;
			}
			catch (NumberFormatException e) {
				showMessageCode(23);	// entered not a number
				valueIsValid = false;
			}
			if (valueIsValid) {
				if (wheelDiameter > 255) {
					showMessageCode(24);	// entered to big value
				}
				else if (wheelDiameter < 1) {
					showMessageCode(25);	// entered to small value
				} else {
					setValueButton.setEnabled(false);
					diameterValue.setCursorVisible(false);
					if ((int) wheelDiameter == wheelDiameter) {
						viewModel.sendWheelDiameter((int) wheelDiameter);
					}
					else {
						// set last bit to '1', so we know there is .5 in the wheel diameter
						viewModel.sendWheelDiameter((int) wheelDiameter | 0b10000000);
					}
				}
			}
		});
		
		viewModel.getConnectionState().observe(this, state -> {
			switch (state.getState()) {
				case CONNECTING:
					progressContainer.setVisibility(View.VISIBLE);
					notSupported.setVisibility(View.GONE);
					connectionState.setText(R.string.state_connecting);
					break;
				case INITIALIZING:
					connectionState.setText(R.string.state_initializing);
					break;
				case READY:
					switch (nbrAddresses) {
						case 1:
							viewModel.sendAddresses(address1);
							break;
						case 2:
							viewModel.sendAddresses(address1);
							viewModel.sendAddresses(address2);
							break;
						case 3:
							viewModel.sendAddresses(address1);
							viewModel.sendAddresses(address2);
							viewModel.sendAddresses(address3);
							break;
						default:
							break;
					}
					//viewModel.sendAddresses(addressesToSend);
					progressContainer.setVisibility(View.GONE);
					content.setVisibility(View.VISIBLE);
					onConnectionStateChanged(true);
					break;
				case DISCONNECTED:
					if (state instanceof ConnectionState.Disconnected) {
						final ConnectionState.Disconnected stateWithReason = (ConnectionState.Disconnected) state;
						if (stateWithReason.isNotSupported()) {
							progressContainer.setVisibility(View.GONE);
							notSupported.setVisibility(View.VISIBLE);
						}
					}
					// fallthrough
				case DISCONNECTING:
					onConnectionStateChanged(false);
					break;
			}
		});


		viewModel.getRPMValue().observe(this,
				integer -> cadenceValue.setText(integer.toString()));

		viewModel.getSpeedValue().observe(this, this::setSpeedValue);

		viewModel.getHeartRateValue().observe(this,
				integer -> heartRateValue.setText(integer.toString()));

		viewModel.getSpeedValue().observe(this, this::setDistance);

		viewModel.getMessageCode().observe(this, this::showMessageCode);

		viewModel.getBatteryLevelSpeed().observe(this, this::setBatteryLevelSpeed);

		viewModel.getBatteryLevelCadence().observe(this, this::setBatteryLevelCadence);

		viewModel.getBatteryLevelHeartRate().observe(this, this::setBatteryLevelHeartRate);
	}

	/**
	 * try to reconnect
	 */
	@OnClick(R.id.action_clear_cache)
	public void onTryAgainClicked() {
		viewModel.reconnect();
	}

	/**
	 * called when the connection state has been changed
	 * @param connected state
	 */
	private void onConnectionStateChanged(final boolean connected) {
		if (!connected) {
			if (firstEntry) {
				firstEntry = false;
				setText("Disconnected from Board");
				setText("Please go 1 step back and reconnect to Board");
			}
			firstEntry = true;
		}
		else {
			setText("Connected with Board");
			firstEntry = true;
		}

	}

	/**
	 * set speed value with maximum two digits after the comma
	 * @param speed the speed value in km/h
	 */
	public void setSpeedValue(Double speed) {
		n2.setMaximumFractionDigits(2);
		speedValue.setText(n2.format(speed));
	}

	/**
	 * calculate the distance
	 * @param speed double speed value
	 */
	public void setDistance(Double speed) {
		double speed_in_km_s = speed / 3600; // speed in km/s
		distance += speed_in_km_s * 1;	// distance in km, * 1s just to say its in km now
		n1.setMaximumFractionDigits(2);
		distanceValue.setText((n1.format(distance)));
	}

	/**
	 * show toast on application
	 * @param msg string to show
	 */
	public void setText(String msg) {
		Toast toast = Toast.makeText(this,msg,Toast.LENGTH_SHORT);
		LinearLayout layout = (LinearLayout) toast.getView();
		if (layout.getChildCount() > 0) {
			TextView tv = (TextView) layout.getChildAt(0);
			tv.setGravity(Gravity.CENTER_VERTICAL | Gravity.CENTER_HORIZONTAL);
		}

		toast.show();
	}

	/**
	 * show the correct error code
	 * @param messageCode the error code
	 */
	public void showMessageCode(Integer messageCode) {
		switch (messageCode) {
			case 10:
				setText("Service not found");
				setText("Retrying...");
				break;
			case 11:
				setText("Disconnected from CSC sensor...");
				break;
			case 12:
				setText("Disconnected from heart rate sensor...");
				break;
			case 13:
				setText("Disconnected from all sensors...");
				break;
			case 14:
				setText("Speed sensor connected");
				setText("Application ready to use");
				setText("Please enter diameter value to start measurement");
				break;
			case 15:
				setText("Cadence sensor connected");
				setText("Application ready to use");
				break;
			case 16:
				setText("First sensor connected");
				setText("Waiting for next sensor...");
				break;
			case 17:
				setText("Second sensor connected");
				setText("Application ready to use");
				setText("Please enter diameter value to start measurement");
				break;
			case 18:
				setText("Second sensor connected");
				setText("Application ready to use");
				break;
			case 19:
				setText("Second sensor connected");
				setText("Waiting for next sensor...");
				break;
			case 20:
				setText("Third sensor connected");
				setText("Application ready to use");
				setText("Please enter diameter value to start measurement");
				break;
			case 21:
				setText("Heart rate sensor connected");
				setText("Application ready to use");
				break;
			case 22:
				setText("Heart rate sensor reconnected");
				break;
			case 23:
				setText("Diameter value must be a number!");
				break;
			case 24:
				setText("Please enter value smaller than 255 Inch");
				break;
			case 25:
				setText("Please enter value bigger than 1 Inch");
				break;
			default:
				break;
		}
	}

	public void setBatteryLevelSpeed(Integer level) {
		batteryLevelSpeed.setText(level.toString() + "%");

		if (level > 10) {
			batteryIconSpeed.setImageResource(R.drawable.battery_full_icon);
		}
		else {
			batteryIconSpeed.setImageResource(R.drawable.battery_alert_icon);
		}
	}

	public void setBatteryLevelCadence(Integer level) {
		batteryLevelCadence.setText(level.toString() + "%");

		if (level > 10) {
			batteryIconCadence.setImageResource(R.drawable.battery_full_icon);
		}
		else {
			batteryIconCadence.setImageResource(R.drawable.battery_alert_icon);
		}
	}

	public void setBatteryLevelHeartRate(Integer level) {
		batteryLevelHeartRate.setText(level.toString() + "%");

		if (level > 10) {
			batteryIconHeartRate.setImageResource(R.drawable.battery_full_icon);
		}
		else {
			batteryIconHeartRate.setImageResource(R.drawable.battery_alert_icon);
		}
	}
}

