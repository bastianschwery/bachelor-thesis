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
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.appbar.MaterialToolbar;

import java.text.NumberFormat;

import butterknife.BindView;
import butterknife.ButterKnife;
import butterknife.OnClick;
import no.nordicsemi.android.ble.livedata.state.ConnectionState;
import no.nordicsemi.android.csc.adapter.DiscoveredBluetoothDevice;
import no.nordicsemi.android.csc.viewmodels.CSCViewModel;

@SuppressWarnings("ConstantConditions")
public class CSCActivity extends AppCompatActivity {
	public static final String EXTRA_DEVICE = "no.nordicsemi.android.csc.EXTRA_DEVICE";

	private boolean firstEntry = false;
	private CSCViewModel viewModel;
	private TextView diameterValue;
	private Button setValueButton, resetButton, resetDistanceButton;
	private double wheelDiameter = 0.0;
	private double distance = 0;
	private NumberFormat n = NumberFormat.getInstance();

	@BindView(R.id.set_button) Button button;
	@BindView(R.id.reset_button) Button rstBtn;
	@BindView(R.id.reset_distance_button) Button rstDistanceBtn;
	@BindView(R.id.speed_value) TextView speedValue;
	@BindView(R.id.cadence_value) TextView cadenceValue;
	@BindView(R.id.distance_value) TextView distanceValue;

	/**
	 * create all necessary instances and add on click listeners
	 * @param savedInstanceState
	 */
	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_csc);
		ButterKnife.bind(this);

		final Intent intent = getIntent();
		final DiscoveredBluetoothDevice device = intent.getParcelableExtra(EXTRA_DEVICE);
		final String deviceName = device.getName();
		final String deviceAddress = device.getAddress();

		final MaterialToolbar toolbar = findViewById(R.id.toolbar);
		toolbar.setTitle(deviceName != null ? deviceName : getString(R.string.unknown_device));
		toolbar.setSubtitle(deviceAddress);
		setSupportActionBar(toolbar);
		getSupportActionBar().setDisplayHomeAsUpEnabled(true);

		// Configure the view model.
		viewModel = new ViewModelProvider(this).get(CSCViewModel.class);
		viewModel.connect(device);

		// Set up views.
		final LinearLayout progressContainer = findViewById(R.id.progress_container);
		final TextView connectionState = findViewById(R.id.connection_state);
		final View content = findViewById(R.id.device_container);
		final View notSupported = findViewById(R.id.not_supported);
		diameterValue = findViewById(R.id.diameter_value);
		setValueButton = findViewById(R.id.set_button);
		resetButton = findViewById(R.id.reset_button);
		resetDistanceButton = findViewById(R.id.reset_distance_button);

		resetDistanceButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				distanceValue.setText("0");
				distance = 0;
			}
		});

		resetButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				setValueButton.setEnabled(true);
				diameterValue.setText("0");
				speedValue.setText("0");
				cadenceValue.setText("0");
				viewModel.resetDiameter();
			}
		});

		setValueButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				wheelDiameter = Double.valueOf(diameterValue.getText().toString());
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

		viewModel.getSpeedValue().observe(this,integer -> setDistance(integer));
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
	 * @param connected
	 */
	private void onConnectionStateChanged(final boolean connected) {
		if (!connected) {
			if (firstEntry) {
				Toast.makeText(this, "Disconnected from Board", Toast.LENGTH_SHORT).show();
			}
			firstEntry = true;
		}
		else {
			Toast.makeText(this, "Connected", Toast.LENGTH_SHORT).show();
			firstEntry = false;
		}
	}

	/**
	 * calculate the distance
	 * @param speed
	 */
	public void setDistance(Double speed) {
		double speed_in_km_s = speed / 3600; // speed in km/s
		distance += speed_in_km_s * 1;	// distance in km, * 1s just to say its in km now
		n.setMaximumFractionDigits(2);
		distanceValue.setText((n.format(distance)));
	}
}
