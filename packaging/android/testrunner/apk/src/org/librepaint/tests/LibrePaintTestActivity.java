/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

package org.librepaint.tests;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import org.qtproject.qt.android.bindings.QtActivity;

public final class LibrePaintTestActivity extends QtActivity
{
    private static void copyAssetTree(AssetManager assets, String source, File destination)
        throws IOException
    {
        final String[] children = assets.list(source);
        if (children != null && children.length > 0) {
            if (!destination.isDirectory() && !destination.mkdirs()) {
                throw new IOException("Cannot create test data directory: " + destination);
            }
            for (String child : children) {
                copyAssetTree(assets, source + "/" + child, new File(destination, child));
            }
            return;
        }

        try (InputStream input = assets.open(source);
             FileOutputStream output = new FileOutputStream(destination)) {
            final byte[] buffer = new byte[16 * 1024];
            int count;
            while ((count = input.read(buffer)) >= 0) {
                output.write(buffer, 0, count);
            }
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        final File filesDirectory = getExternalFilesDir(null);
        if (filesDirectory == null) {
            throw new IllegalStateException("Android test result directory is unavailable");
        }
        try {
            final String[] testData = getAssets().list("data");
            if (testData != null && testData.length > 0) {
                copyAssetTree(getAssets(), "data", new File(filesDirectory, "data"));
            }
            Os.setenv("LIBREPAINT_ANDROID_TEST_FILES", filesDirectory.getAbsolutePath(), true);
        } catch (ErrnoException | IOException error) {
            throw new IllegalStateException("Android test runtime files are unavailable", error);
        }
        super.onCreate(savedInstanceState);
    }
}
