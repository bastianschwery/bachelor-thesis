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

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.provider.Settings;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SimpleItemAnimator;

import com.google.android.material.appbar.MaterialToolbar;

import java.util.ArrayList;

import butterknife.BindView;
import butterknife.ButterKnife;
import butterknife.OnClick;
import no.nordicsemi.android.csc.adapter.DevicesAdapter;
import no.nordicsemi.android.csc.adapter.DiscoveredBluetoothDevice;
import no.nordicsemi.android.csc.utils.Utils;
import no.nordicsemi.android.csc.viewmodels.ScannerStateLiveData;
import no.nordicsemi.android.csc.viewmodels.ScannerViewModel;

public class ScannerActivity extends AppCompatActivity implements DevicesAdapter.OnItemClickListener {
    private static final int REQUEST_ACCESS_FINE_LOCATION = 1022; // random number

    // change this values when using other CSC / heart rate sensors (also in CSCActivity)
    private final String BOARD_NAME = "Nordic";
    private final String SPEED_NAME = "SPD";
    private final String CADENCE_NAME = "CAD";
    private final String HEARTRATE_NAME = "Polar";

    private ScannerViewModel scannerViewModel;
    private ArrayList<DiscoveredBluetoothDevice> devices_list = new ArrayList<>();
    private DiscoveredBluetoothDevice[] devices;
    private boolean boardSelected = false;
    private boolean wrongSensors = false;
    private boolean nbrDevicesOK = false;
    private boolean nbrSensorsOK = false;
    private Button connect_btn;
    private int cntGoodDevices = 0;

    @BindView(R.id.state_scanning) View scanningView;
    @BindView(R.id.no_devices) View emptyView;
    @BindView(R.id.no_location_permission) View noLocationPermissionView;
    @BindView(R.id.action_grant_location_permission) Button grantPermissionButton;
    @BindView(R.id.action_permission_settings) Button permissionSettingsButton;
    @BindView(R.id.no_location) View noLocationView;
    @BindView(R.id.bluetooth_off) View noBluetoothView;

    /**
     * create all necessary instances and add on click listeners
     * @param savedInstanceState state
     */
    @Override
    protected void onCreate(@Nullable final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_scanner);
        ButterKnife.bind(this);

        final MaterialToolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setTitle(R.string.app_name);
        setSupportActionBar(toolbar);

        connect_btn = findViewById(R.id.connect_button);
        connect_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (devices_list.isEmpty()) {
                    setText("Please select minimum one device");
                }
                else
                {
                    connect();
                }
            }
        });

        // Create view model containing utility methods for scanning
        scannerViewModel = new ViewModelProvider(this).get(ScannerViewModel.class);
        scannerViewModel.getScannerState().observe(this, this::startScan);

        // Configure the recycler view
        final RecyclerView recyclerView = findViewById(R.id.recycler_view_ble_devices);
        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        recyclerView.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));

        final RecyclerView.ItemAnimator animator = recyclerView.getItemAnimator();
        if (animator instanceof SimpleItemAnimator) {
            ((SimpleItemAnimator) animator).setSupportsChangeAnimations(false);
        }

        final DevicesAdapter adapter = new DevicesAdapter(this, scannerViewModel.getDevices());
        adapter.setOnItemClickListener(this);
        recyclerView.setAdapter(adapter);
    }

    // check if known sensors and a board is selected
    private void connect() {
        if (devices_list.size() > 4) {
            nbrDevicesOK = false;
            setText("Please select not more than 4 devices");
        }
        else {
            nbrDevicesOK = true;
        }

        for (int i=0;i<devices_list.size();i++) {
            // check if a unknown device is selected
            if (devices_list.get(i).getName() == null) {
                setText("Unknown sensor/s were selected -> try again");
                wrongSensors = true;
                break;
            }

            // count if there are known and accepted sensors are selected
            if (devices_list.get(i).getName().contains(SPEED_NAME) || devices_list.get(i).getName().contains(CADENCE_NAME)
                || devices_list.get(i).getName().contains(HEARTRATE_NAME) || devices_list.get(i).getName().contains(BOARD_NAME)) {
                cntGoodDevices++;
            }

            // check if a board is selected
            if (devices_list.get(i).getName().contains(BOARD_NAME)) {
                boardSelected = true;
            }
        }

        // check if there is a not accepted device selected
        if (cntGoodDevices != devices_list.size() && !wrongSensors) {
            setText("Wrong sensor/s were selected -> try again");
            cntGoodDevices = 0;
            wrongSensors = true;
        }

        // check if there is at minimum one accepted sensor selected
        if (boardSelected && devices_list.size() == 1) {
            setText("Please select minimum one sensor");
            nbrSensorsOK = false;
        }
        else {
            nbrSensorsOK = true;
        }

        // if all checks are good, start next and send list with devices to CSCActivity
        if (boardSelected && nbrDevicesOK && nbrSensorsOK && !wrongSensors) {
            boardSelected = false;
            nbrDevicesOK = false;
            nbrSensorsOK = false;
            wrongSensors = false;
            cntGoodDevices = 0;
            devices = new DiscoveredBluetoothDevice[devices_list.size()];
            devices = devices_list.toArray(devices);
            final Intent controlBlinkIntent = new Intent(this, CSCActivity.class);
            controlBlinkIntent.putExtra(CSCActivity.EXTRA_DEVICE, devices);
            startActivity(controlBlinkIntent);
        }   // otherwise reset values for next try
        else if (!boardSelected && !wrongSensors) {
            setText("Please select a Nordic Board to continue");
            wrongSensors = false;
            nbrSensorsOK = false;
            boardSelected = false;
            nbrDevicesOK = false;
            cntGoodDevices = 0;
        }
        else {
            wrongSensors = false;
            nbrSensorsOK = false;
            boardSelected = false;
            nbrDevicesOK = false;
            cntGoodDevices = 0;
        }
    }

    /**
     * on restart application -> call constructor and clear
     */
    @Override
    protected void onRestart() {
        super.onRestart();
        clear();
    }

    /**
     * when stopping application -> stop scanning
     */
    @Override
    protected void onStop() {
        super.onStop();
        devices_list.clear();
        stopScan();
    }

    /**
     * create options menu
     * @param menu object
     * @return true
     */
    @Override
    public boolean onCreateOptionsMenu(final Menu menu) {
        getMenuInflater().inflate(R.menu.filter, menu);
        menu.findItem(R.id.filter_uuid).setChecked(scannerViewModel.isUuidFilterEnabled());
        menu.findItem(R.id.filter_nearby).setChecked(scannerViewModel.isNearbyFilterEnabled());
        return true;
    }

    /**
     * get the selected menu options
     * @param item object
     * @return true
     */
    @Override
    public boolean onOptionsItemSelected(final MenuItem item) {
        switch (item.getItemId()) {
            case R.id.filter_uuid:
                item.setChecked(!item.isChecked());
                scannerViewModel.filterByUuid(item.isChecked());
                return true;
            case R.id.filter_nearby:
                item.setChecked(!item.isChecked());
                scannerViewModel.filterByDistance(item.isChecked());
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * get clicked item
     * @param device item
     * @param set
     */
    @Override
    public void onItemClick(@NonNull final DiscoveredBluetoothDevice device, boolean set) {
        if (set) {
            devices_list.add(device);
        }
        else {
            devices_list.remove(device);
        }
    }

    /**
     * result of the permission request
     * @param requestCode int code
     * @param permissions String array
     * @param grantResults int array
     */
    @Override
    public void onRequestPermissionsResult(final int requestCode,
                                           @NonNull final String[] permissions,
                                           @NonNull final int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_ACCESS_FINE_LOCATION) {
            scannerViewModel.refresh();
        }
    }

    /**
     * when location enabled clicked
     */
    @OnClick(R.id.action_enable_location)
    public void onEnableLocationClicked() {
        final Intent intent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
        startActivity(intent);
    }

    /**
     * when bluetooth enable clicked
     */
    @OnClick(R.id.action_enable_bluetooth)
    public void onEnableBluetoothClicked() {
        final Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
        startActivity(enableIntent);
    }

    /**
     * when grant location enable clicked
     */
    @OnClick(R.id.action_grant_location_permission)
    public void onGrantLocationPermissionClicked() {
        Utils.markLocationPermissionRequested(this);
        ActivityCompat.requestPermissions(
                this,
                new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                REQUEST_ACCESS_FINE_LOCATION);
    }

    /**
     * when permission settings clicked
     */
    @OnClick(R.id.action_permission_settings)
    public void onPermissionSettingsClicked() {
        final Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
        intent.setData(Uri.fromParts("package", getPackageName(), null));
        startActivity(intent);
    }

    /**
     * Start scanning for Bluetooth devices or displays a message based on the scanner state.
     */
    private void startScan(final ScannerStateLiveData state) {
        // First, check the Location permission. This is required on Marshmallow onwards in order
        // to scan for Bluetooth LE devices.
        if (Utils.isLocationPermissionsGranted(this)) {
            noLocationPermissionView.setVisibility(View.GONE);

            // Bluetooth must be enabled.
            if (state.isBluetoothEnabled()) {
                noBluetoothView.setVisibility(View.GONE);

                // We are now OK to start scanning.
                scannerViewModel.startScan();
                scanningView.setVisibility(View.VISIBLE);

                if (!state.hasRecords()) {
                    emptyView.setVisibility(View.VISIBLE);

                    if (!Utils.isLocationRequired(this) || Utils.isLocationEnabled(this)) {
                        noLocationView.setVisibility(View.INVISIBLE);
                    } else {
                        noLocationView.setVisibility(View.VISIBLE);
                    }
                } else {
                    emptyView.setVisibility(View.GONE);
                }
            } else {
                noBluetoothView.setVisibility(View.VISIBLE);
                scanningView.setVisibility(View.INVISIBLE);
                emptyView.setVisibility(View.GONE);
                clear();
            }
        } else {
            noLocationPermissionView.setVisibility(View.VISIBLE);
            noBluetoothView.setVisibility(View.GONE);
            scanningView.setVisibility(View.INVISIBLE);
            emptyView.setVisibility(View.GONE);

            final boolean deniedForever = Utils.isLocationPermissionDeniedForever(this);
            grantPermissionButton.setVisibility(deniedForever ? View.GONE : View.VISIBLE);
            permissionSettingsButton.setVisibility(deniedForever ? View.VISIBLE : View.GONE);
        }
    }

    /**
     * stop scanning for bluetooth devices.
     */
    private void stopScan() {
        scannerViewModel.stopScan();
    }

    /**
     * Clears the list of devices, which will notify the observer.
     */
    private void clear() {
        devices_list.clear();
        scannerViewModel.getDevices().clear();
        scannerViewModel.getScannerState().clearRecords();
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
}