// SPDX-License-Identifier: GPL-3.0-or-later
package org.krita.android;

import android.app.Activity;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.Html;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.krita.R;

import java.util.List;

public class DonationProductView {
    private static final String TAG = "krita.DonationProduct";

    View mView;

    public DonationProductView(Activity activity, Drawable leftImageDrawable,
                               Drawable rightImageDrawable, String description, String buttonText,
                               View.OnClickListener onButtonClick) {
        mView = activity.getLayoutInflater().inflate(R.layout.donation_item_layout, null);

        LinearLayout productLayout = mView.findViewById(R.id.productLayout);

        ImageView leftImage = mView.findViewById(R.id.leftImage);
        if (leftImageDrawable == null) {
            productLayout.removeView(leftImage);
        } else {
            leftImage.setImageDrawable(makeSmoothDrawable(leftImageDrawable));
            leftImage.setOnClickListener(onButtonClick);
        }

        ImageView rightImage = mView.findViewById(R.id.rightImage);
        if (rightImageDrawable == null) {
            productLayout.removeView(rightImage);
        } else {
            rightImage.setImageDrawable(makeSmoothDrawable(rightImageDrawable));
            rightImage.setOnClickListener(onButtonClick);
        }

        TextView productDescription = mView.findViewById(R.id.productDescription);
        productDescription.setText(Html.fromHtml(description, Html.FROM_HTML_MODE_COMPACT));

        Button productButton = mView.findViewById(R.id.productButton);
        productButton.setText(buttonText);
        productButton.setOnClickListener(onButtonClick);
    }

    public View getView() {
        return mView;
    }

    public static List<DonationProductView> getFallbackProducts(Activity activity) {
        return List.of();
    }

    private static Drawable makeSmoothDrawable(Drawable drawable) {
        if (drawable instanceof BitmapDrawable) {
            try {
                BitmapDrawable bd = (BitmapDrawable) drawable.mutate();
                bd.setAntiAlias(true);
                bd.setFilterBitmap(true);
                return bd;
            } catch (Exception e) {
                Log.e(TAG, "Exception mutating drawable", e);
            }
        }
        return drawable;
    }
}
