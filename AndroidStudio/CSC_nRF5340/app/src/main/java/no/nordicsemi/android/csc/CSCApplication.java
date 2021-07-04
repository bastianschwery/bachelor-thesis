package no.nordicsemi.android.csc;

import android.app.Application;
import android.os.Build;

import androidx.appcompat.app.AppCompatDelegate;

public class CSCApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        // Added to support vector drawables for devices below Android 21.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            AppCompatDelegate.setCompatVectorFromResourcesEnabled(true);
        }
    }
}
