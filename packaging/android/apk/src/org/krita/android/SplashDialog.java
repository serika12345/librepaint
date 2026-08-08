// SPDX-License-Identifier: GPL-3.0-or-later
package org.krita.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Bitmap;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import org.krita.R;

public final class SplashDialog {
    private final View mView;
    private final AlertDialog mAlertDialog;
    private boolean mLoading = true;

    public SplashDialog(Activity activity) {
        mView = activity.getLayoutInflater().inflate(R.layout.splash_dialog_layout, null);
        mAlertDialog = new AlertDialog.Builder(activity)
                .setView(mView)
                .create();
        mAlertDialog.setCancelable(false);
    }

    public AlertDialog getAlertDialog() {
        return mAlertDialog;
    }

    public void setLoading(boolean loading) {
        if (mLoading == loading) {
            return;
        }

        mLoading = loading;
        mAlertDialog.setCancelable(!loading);
        if (!loading) {
            mAlertDialog.dismiss();
        }
    }

    public void setLoadingText(String text) {
        TextView loadingText = mView.findViewById(R.id.loadingText);
        loadingText.setText(text);
    }

    public void setSplashContents(Bitmap bitmap, String text) {
        ImageView splashImage = mView.findViewById(R.id.splashImage);
        splashImage.setImageBitmap(bitmap);

        TextView splashText = mView.findViewById(R.id.splashText);
        splashText.setText(text);
    }
}
