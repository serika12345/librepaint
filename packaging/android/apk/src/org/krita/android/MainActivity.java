/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Sharaf Zaman <sharafzaz121@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

package org.krita.android;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.ApplicationExitInfo;
import android.app.ForegroundServiceStartNotAllowedException;
import android.app.ServiceStartNotAllowedException;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewConfiguration;

import androidx.annotation.RequiresApi;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import java.util.List;

import org.krita.R;
import org.libsdl.app.SDLAudioManager;
import org.qtproject.qt.android.bindings.QtActivity;

import java.util.function.Consumer;

public class MainActivity extends QtActivity {

    private static final String TAG = "krita.MainActivity";
    private static volatile MainActivity currentActivity = null;
    private static boolean applicationLoaded = false;
    private static String applicationLoadingText = "";
    private boolean haveLibsLoaded = false;
    private boolean serviceStarted = false;
    private boolean inFullScreen = false;
    private SplashDialog mSplashDialog = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        // Qt 6.11.1 can abort while creating a second OpenGL-backed top-level
        // surface if its Android accessibility bridge is waiting for the Qt
        // event loop. This must be set before QtActivity initializes the
        // bridge. Remove it after the minimum Qt version supports multi-window
        // surface creation with an active Android accessibility service.
        try {
            Os.setenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1", true);
        } catch (ErrnoException error) {
            throw new IllegalStateException(
                    "Unable to apply the Qt Android accessibility workaround", error);
        }

        currentActivity = this;

        // we have to do this before loading main()
        Intent i = getIntent();
        String uri = getUri(i);
        if (uri != null) {
            // this will be passed as a command line argument to main()
            i.putExtra("applicationArguments", uri);
        }

        SDLAudioManager.initialize();
        SDLAudioManager.setContext(this);
        SDLAudioManager.nativeSetupJNI();

        super.onCreate(savedInstanceState);
        Log.i(TAG, "TouchSlop: " + ViewConfiguration.get(this).getScaledTouchSlop());
        Log.i(TAG, "LibsLoaded");
        haveLibsLoaded = true;
    }

    @Override
    public void onStart() {
        super.onStart();

        // unlike onCreate where we did this before, this method is called several times throughout the
        // lifecycle of our app, but we intend to run this method only once (and in "Foreground").
        if (!serviceStarted) {
            serviceStarted  = true;
            // Full-screening the application here instead of after the main window is shown avoids
            // some ugly flicker as the Qt UI resizes itself.
            trySetFullScreen(true);
            // Keep the service started so in an unfortunate case where we're not allowed to start a
            // foreground service, we can try to continue without it.
            Intent docSaverServiceIntent = new Intent(this, DocumentSaverService.class);
            startService(docSaverServiceIntent);
        }
    }

    @SuppressLint("MissingSuperCall")
    @Override
    protected void onNewIntent (Intent intent) {
        // LibrePaint owns launcher and document intents. Keep the Activity's
        // current intent in sync without forwarding it to Qt's native intent
        // listeners, whose platform-service lifetime can end on an Activity
        // configuration change.
        setIntent(intent);

        String uri = getUri(intent);
        if (uri != null) {
            JNIWrappers.openFileFromIntent(uri);
        }
    }

    private String getUri(Intent intent) {
        if (intent != null) {
            Uri fileUri = intent.getData();
            if (fileUri != null) {
                return fileUri.toString();
            }
        }
        return null;
    }

    @Override
    public void onPause() {
        super.onPause();
        // onPause() _is_ called when the app starts. If the native lib
        // isn't loaded, it crashes.
        if (haveLibsLoaded) {
            synchronized(this) {
                startServiceGeneric(DocumentSaverService.START_SAVING);
            }
        }
    }

    void startServiceGeneric(final String action) {
        Intent intent = new Intent(this, DocumentSaverService.class);
        intent.putExtra(action, true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            startForegroundServiceS(intent);
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.S)
    void startForegroundServiceS(Intent intent) {
        try {
            try {
                startForegroundService(intent);
            } catch (ForegroundServiceStartNotAllowedException e) {
                Log.w(TAG, "ForegroundServiceStartNotAllowedException: " + e);

                // The service is already running, so maybe try saving without trying to put it in
                // foreground. According to docs we should have a couple of minutes of runtime.
                startService(intent);
            }
        } catch (ServiceStartNotAllowedException e) {
            // We may not be allowed to start a background service either,
            // probably because onPause is called on an already-paused
            // application that is beyond the "couple of minutes" cutoff.
            Log.w(TAG, "ServiceStartNotAllowedException: " + e);
        }
    }

    @Override
    public void onDestroy() {
        // Docs say: this method will not be called if the activity's hosting process
        // is killed. This means, for us that the service has been stopped.

        Log.i(TAG, "[onDestroy]");
        startServiceGeneric(DocumentSaverService.KILL_PROCESS);

        super.onDestroy();
        if (currentActivity == this) {
            currentActivity = null;
        }
    }

    @Override
    public boolean onKeyUp(final int keyCode, final KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (!JNIWrappers.hasMainWindowLoaded()) {
                // back button was pressed during splash screen, letting this
                // propagate leaves native side in an undefined state. So, it's
                // best we finish the activity here.
                finish();
            }
        }

        return super.onKeyUp(keyCode, event);
    }

    public void onUserInteraction() {
    }

    public void copyAssets() {
        new ConfigsManager(this).handleAssets();
    }

    public boolean isInFullScreen() {
        return inFullScreen;
    }

    public void setFullScreenOnUiThread(boolean fullScreen) {
        if (fullScreen != inFullScreen) {
            runOnUiThread(() -> trySetFullScreen(fullScreen));
        }
    }

    private void trySetFullScreen(boolean fullScreen) {
        try {
            View decorView = getWindow().getDecorView();
            WindowInsetsControllerCompat controller =
                    WindowCompat.getInsetsController(getWindow(), decorView);
            if (fullScreen) {
                WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
                controller.setSystemBarsBehavior(
                        WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                controller.hide(WindowInsetsCompat.Type.systemBars());
            } else {
                controller.show(WindowInsetsCompat.Type.systemBars());
                WindowCompat.setDecorFitsSystemWindows(getWindow(), true);
            }
            inFullScreen = fullScreen;
        } catch (Exception | UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to set fullscreen " + fullScreen, e);
        }
    }

    public static int getLongPressTimeout() {
        try {
            MainActivity activity = currentActivity;
            return activity == null ? 500 : ViewConfiguration.get(activity).getLongPressTimeout();
        } catch (Exception|UnsatisfiedLinkError e) {
            Log.e(TAG, "Exception getting long press timeout", e);
            return 500;
        }
    }

    public static boolean looksLikeXiaomiDevice() {
        return containsXiaomi(Build.BRAND) || containsXiaomi(Build.MANUFACTURER);
    }

    private static boolean containsXiaomi(String s) {
        return s != null && s.toLowerCase().contains("xiaomi");
    }

    public ApplicationExitInfo getLastApplicationExitInfo() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            try {
                ActivityManager activityManager = getSystemService(ActivityManager.class);
                if (activityManager != null) {
                    List<ApplicationExitInfo> exitReasons =
                        activityManager.getHistoricalProcessExitReasons(null, 0, 1);
                    if (exitReasons != null && !exitReasons.isEmpty()) {
                        return exitReasons.get(0);
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Exception getting last application exit info", e);
            }
        }
        return null;
    }

    public static boolean isLowMemoryKillReportSupported() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            try {
                return ActivityManager.isLowMemoryKillReportSupported();
            } catch (Exception e) {
                Log.e(TAG, "Exception getting low memory kill report support", e);
            }
        }
        return false;
    }

    @SuppressWarnings("unused")
    public static void showSplashDialog(byte[] splashBytes, String splashVersion) {
        Log.d(TAG, "showSplashDialog");
        try {
            MainActivity activity = currentActivity;
            if (activity != null) {
                activity.showSplashDialogInternal(splashBytes, splashVersion);
            } else {
                Log.e(TAG, "showSplashDialog: the LibrePaint activity is unavailable");
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception dispatching splash dialog", e);
        }
    }

    private void showSplashDialogInternal(byte[] splashBytes, String splashVersion) {
        runOnUiThread(() -> {
            if (mSplashDialog == null) {
                try {
                    mSplashDialog = new SplashDialog(MainActivity.this);
                    AlertDialog alertDialog = mSplashDialog.getAlertDialog();
                    alertDialog.setOnDismissListener(dialogInterface -> {
                        JNIWrappers.onSplashDialogDismissed();
                        if (mSplashDialog != null) {
                            if (dialogInterface == mSplashDialog.getAlertDialog()) {
                                Log.d(TAG, "Splash dialog dismissed, clearing");
                                mSplashDialog = null;
                            } else {
                                Log.w(TAG, "Unknown splash dialog dismissed, not clearing it");
                            }
                        }
                    });
                    alertDialog.show();

                    Bitmap bitmap = loadSplashImageBitmap(splashBytes);
                    if (bitmap != null) {
                        mSplashDialog.setSplashContents(bitmap, splashVersion);
                    }
                    updateSplashDialogInternal();
                } catch (Exception e) {
                    Log.e(TAG, "Exception showing splash dialog", e);
                }
            } else {
                Log.w(TAG, "Splash dialog requested while already shown");
            }
        });
    }

    private static Bitmap loadSplashImageBitmap(byte[] splashBytes) {
        if (splashBytes == null || splashBytes.length == 0) {
            Log.d(TAG, "No splash image data given");
            return null;
        }

        Bitmap bitmap;
        try {
            Log.d(TAG, "Loading splash image bitmap");
            bitmap = BitmapFactory.decodeByteArray(splashBytes, 0, splashBytes.length);
        } catch (Exception e) {
            Log.d(TAG, "Exception loading splash image bitmap", e);
            return null;
        }

        if (bitmap == null) {
            Log.w(TAG, "Null splash image bitmap loaded");
            return null;
        }

        Log.d(TAG, "Loaded splash image bitmap");
        return bitmap;
    }

    @SuppressWarnings("unused")
    public static void setLoaded(boolean loaded) {
        Log.d(TAG, "setLoaded " + loaded);
        applicationLoaded = loaded;
        updateSplashDialog();
    }

    @SuppressWarnings("unused")
    public static void setLoadingText(String loadingText) {
        Log.d(TAG, "setLoadingText " + loadingText);
        applicationLoadingText = loadingText;
        updateSplashDialog();
    }

    private static void updateSplashDialog() {
        doWithMainActivity(MainActivity::updateSplashDialogInternal);
    }

    private void updateSplashDialogInternal() {
        runOnUiThread(() -> {
            if (mSplashDialog == null) {
                Log.d(TAG, "Splash dialog not set, not updating it");
                return;
            }

            mSplashDialog.setLoadingText(getLoadingText());
            mSplashDialog.setLoading(!applicationLoaded);
        });
    }

    private String getLoadingText() {
        if (applicationLoaded) {
            return getString(R.string.splash_dialog_loaded);
        } else if (applicationLoadingText == null || applicationLoadingText.isEmpty()) {
            return getString(R.string.splash_dialog_loading);
        } else {
            return applicationLoadingText;
        }
    }

    public static void doWithMainActivity(Consumer<MainActivity> consumer) {
        try {
            MainActivity activity = currentActivity;
            if (activity != null) {
                consumer.accept(activity);
            } else {
                Log.e(TAG, "doWithMainActivity: the LibrePaint activity is unavailable");
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception in doWithMainActivity", e);
        }
    }

    public void showScalingDialog(double currentScale, double defaultScale, boolean showOnStartup, boolean canShowOnStartup) {
        runOnUiThread(() -> {
            ScalingDialog scalingDialog = new ScalingDialog(this, currentScale, defaultScale, showOnStartup, canShowOnStartup);
            scalingDialog.show();
        });
    }
}
