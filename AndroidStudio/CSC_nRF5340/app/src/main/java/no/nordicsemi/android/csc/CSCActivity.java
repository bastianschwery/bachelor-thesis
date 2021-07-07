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

import android.content.Intent;
import android.os.Bundle;
import android.os.Parcelable;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.appbar.MaterialToolbar;

import java.nio.ByteBuffer;
import java.text.NumberFormat;
import java.util.ArrayList;

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
	private double wheelDiameter = 0.0;
	private double distance = 0;
	private NumberFormat n = NumberFormat.getInstance();
	private ArrayList<String> addresses = new ArrayList<>();
	private DiscoveredBluetoothDevice nordicBoard;
	private ArrayList<DiscoveredBluetoothDevice> devices = new ArrayList<>();
	private Parcelable[] receivedArray = new Parcelable[10];
	byte[] address1;
	byte[] address2;
	byte[] address3;
	byte[] addressesToSend;

	@BindView(R.id.set_button) Button button;
	@BindView(R.id.reset_button) Button rstBtn;
	@BindView(R.id.reset_distance_button) Button rstDistanceBtn;
	@BindView(R.id.speed_value) TextView speedValue;
	@BindView(R.id.cadence_value) TextView cadenceValue;
	@BindView(R.id.distance_value) TextView distanceValue;

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
					addresses.add(devices.get(i).getAddress());
				}
			}
		}


		//final DiscoveredBluetoothDevice device = intent.getParcelableExtra(EXTRA_DEVICE);

		ByteBuffer buffer = ByteBuffer.allocate(addresses.size()*17);

		switch (addresses.size()) {
			case 1:
				address1 = addresses.get(0).getBytes();
				buffer.put(address1);
				break;
			case 2:
				address1 = addresses.get(0).getBytes();
				address2 = addresses.get(1).getBytes();
				buffer.put(address1);
				buffer.put(address2);
				break;
			case 3:
				address1 = addresses.get(0).getBytes();
				address2 = addresses.get(1).getBytes();
				address3 = addresses.get(2).getBytes();
				buffer.put(address1);
				buffer.put(address2);
				buffer.put(address3);
				break;
			default:
				break;
		}

		addressesToSend = buffer.array();


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

		resetDistanceButton.setOnClickListener(v -> {
			distanceValue.setText("0");
			distance = 0;
		});

		resetButton.setOnClickListener(v -> {
			setValueButton.setEnabled(true);
			diameterValue.setText("0");
			speedValue.setText("0");
			cadenceValue.setText("0");
			viewModel.resetDiameter();
		});

		setValueButton.setOnClickListener(v -> {
			wheelDiameter = Double.parseDouble(diameterValue.getText().toString());
			if (wheelDiameter > 255) {
				setText("Please enter value smaller than 255");
			}
			else {
				setValueButton.setEnabled(false);
				if ((int) wheelDiameter == wheelDiameter) {
					viewModel.setWheelDiameter((int) wheelDiameter);
				}
				else {
					// set last bit to '1', so we know there is .5 in the wheel diameter
					viewModel.setWheelDiameter((int) wheelDiameter | 0b10000000);
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
					viewModel.sendAddresses(addressesToSend);
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

		viewModel.getSpeedValue().observe(this,
				integer -> speedValue.setText(integer.toString()));

		viewModel.getSpeedValue().observe(this, this::setDistance);

		viewModel.getMessageCode().observe(this, this::showMessageCode);
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
				setText("Disconnected from Board");
			}
			firstEntry = true;
		}
		else {
			setText("Connected with Board");
			firstEntry = false;
		}
	}

	/**
	 * calculate the distance
	 * @param speed double speed value
	 */
	public void setDistance(Double speed) {
		double speed_in_km_s = speed / 3600; // speed in km/s
		distance += speed_in_km_s * 1;	// distance in km, * 1s just to say its in km now
		n.setMaximumFractionDigits(2);
		distanceValue.setText((n.format(distance)));
	}

	/**
	 * show toast on application
	 * @param msg string to show
	 */
	public void setText(String msg) {
		Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
	}

	/**
	 * show the correct error code
	 * @param messageCode the error code
	 */
	public void showMessageCode(Integer messageCode) {
		switch (messageCode) {
			case 10:
				setText("Service not found");
				break;
			case 11:
				setText("Disconnected from one CSC sensor...");
				break;
			case 12:
				setText("Disconnected from both CSC sensors...");
				break;
			case 13:
				setText("First sensor connected");
				break;
			case 14:
				setText("Second sensor connected");
				setText("Application ready to use");
				setText("Please enter diameter value to start measurement");
				break;
			default:
				break;
		}
	}
}
