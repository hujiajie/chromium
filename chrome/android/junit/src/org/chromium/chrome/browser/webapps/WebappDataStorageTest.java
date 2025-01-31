// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;

import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.testing.local.BackgroundShadowAsyncTask;
import org.chromium.testing.local.LocalRobolectricTestRunner;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import java.util.concurrent.TimeUnit;

/**
 * Tests the WebappDataStorage class by ensuring that it persists data to
 * SharedPreferences as expected.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE, shadows = {BackgroundShadowAsyncTask.class})
public class WebappDataStorageTest {

    private SharedPreferences mSharedPreferences;
    private boolean mCallbackCalled;

    private class FetchCallback<T> implements WebappDataStorage.FetchCallback<T> {
        T mExpected;

        FetchCallback(T expected) {
            mExpected = expected;
        }

        @Override
        public void onDataRetrieved(T readObject) {
            mCallbackCalled = true;
            assertEquals(mExpected, readObject);
        }
    }

    private static class TestClock extends WebappDataStorage.Clock {
        private long mCurrentTime;

        public TestClock(long currentTime) {
            updateTime(currentTime);
        }

        public void updateTime(long currentTime) {
            mCurrentTime = currentTime;
        }

        @Override
        public long currentTimeMillis() {
            return mCurrentTime;
        }
    }

    @Before
    public void setUp() throws Exception {
        mSharedPreferences = Robolectric.application
                .getSharedPreferences("webapp_test", Context.MODE_PRIVATE);

        // Set the last_used as if the web app had been registered by WebappRegistry.
        mSharedPreferences.edit().putLong("last_used", 0).commit();

        mCallbackCalled = false;
    }

    @Test
    @Feature({"Webapp"})
    public void testBackwardCompat() {
        assertEquals("webapp_", WebappDataStorage.SHARED_PREFS_FILE_PREFIX);
        assertEquals("splash_icon", WebappDataStorage.KEY_SPLASH_ICON);
        assertEquals("last_used", WebappDataStorage.KEY_LAST_USED);
        assertEquals("url", WebappDataStorage.KEY_URL);
        assertEquals("scope", WebappDataStorage.KEY_SCOPE);
        assertEquals("icon", WebappDataStorage.KEY_ICON);
        assertEquals("name", WebappDataStorage.KEY_NAME);
        assertEquals("short_name", WebappDataStorage.KEY_SHORT_NAME);
        assertEquals("orientation", WebappDataStorage.KEY_ORIENTATION);
        assertEquals("theme_color", WebappDataStorage.KEY_THEME_COLOR);
        assertEquals("background_color", WebappDataStorage.KEY_BACKGROUND_COLOR);
        assertEquals("source", WebappDataStorage.KEY_SOURCE);
        assertEquals("action", WebappDataStorage.KEY_ACTION);
        assertEquals("is_icon_generated", WebappDataStorage.KEY_IS_ICON_GENERATED);
        assertEquals("version", WebappDataStorage.KEY_VERSION);
    }

    @Test
    @Feature({"Webapp"})
    public void testLastUsedRetrieval() throws Exception {
        mSharedPreferences.edit()
                .putLong(WebappDataStorage.KEY_LAST_USED, 100L)
                .commit();

        WebappDataStorage.getLastUsedTime(Robolectric.application, "test",
                new FetchCallback<Long>(new Long(100L)));
        BackgroundShadowAsyncTask.runBackgroundTasks();
        Robolectric.runUiThreadTasks();

        assertTrue(mCallbackCalled);
    }

    @Test
    @Feature({"Webapp"})
    public void testSplashImageRetrieval() throws Exception {
        final Bitmap expected = createBitmap();
        mSharedPreferences.edit()
                .putString(WebappDataStorage.KEY_SPLASH_ICON,
                        ShortcutHelper.encodeBitmapAsString(expected))
                .commit();
        WebappDataStorage.open(Robolectric.application, "test")
                .getSplashScreenImage(new WebappDataStorage.FetchCallback<Bitmap>() {
                    @Override
                    public void onDataRetrieved(Bitmap actual) {
                        mCallbackCalled = true;

                        // TODO(lalitm) - once the Robolectric bug is fixed change to
                        // assertTrue(expected.sameAs(actual)).
                        // See bitmapEquals(Bitmap, Bitmap) for more information.
                        assertTrue(bitmapEquals(expected, actual));
                    }
                });
        BackgroundShadowAsyncTask.runBackgroundTasks();
        Robolectric.runUiThreadTasks();

        assertTrue(mCallbackCalled);
    }

    @Test
    @Feature({"Webapp"})
    public void testSplashImageUpdate() throws Exception {
        final Bitmap expectedImage = createBitmap();
        WebappDataStorage.open(Robolectric.application, "test")
                .updateSplashScreenImage(expectedImage);
        BackgroundShadowAsyncTask.runBackgroundTasks();

        assertEquals(ShortcutHelper.encodeBitmapAsString(expectedImage),
                mSharedPreferences.getString(WebappDataStorage.KEY_SPLASH_ICON, null));
    }

    @Test
    @Feature({"Webapp"})
    public void testScopeRetrieval() throws Exception {
        final String scope = "http://drive.google.com";
        mSharedPreferences.edit()
                .putString(WebappDataStorage.KEY_SCOPE, scope)
                .commit();

        WebappDataStorage.getScope(Robolectric.application, "test",
                new FetchCallback<String>(scope));
        BackgroundShadowAsyncTask.runBackgroundTasks();
        Robolectric.runUiThreadTasks();

        assertTrue(mCallbackCalled);
    }

    @Test
    @Feature({"Webapp"})
    public void testUrlRetrieval() throws Exception {
        final String url = "https://www.google.com";
        mSharedPreferences.edit()
                .putString(WebappDataStorage.KEY_URL, url)
                .commit();

        WebappDataStorage.getUrl(Robolectric.application, "test",
                new FetchCallback<String>(url));
        BackgroundShadowAsyncTask.runBackgroundTasks();
        Robolectric.runUiThreadTasks();

        assertTrue(mCallbackCalled);
    }

    @Test
    @Feature({"Webapp"})
    public void testWasLaunchedRecently() throws Exception {
        final TestClock clock = new TestClock(System.currentTimeMillis());
        WebappDataStorage.setClockForTests(clock);

        WebappDataStorage storage = WebappDataStorage.open(Robolectric.application, "test");
        storage.updateLastUsedTime();
        BackgroundShadowAsyncTask.runBackgroundTasks();
        Robolectric.runUiThreadTasks();
        assertTrue(!storage.wasLaunchedRecently());

        long lastUsedTime = mSharedPreferences.getLong(WebappDataStorage.KEY_LAST_USED,
                WebappDataStorage.LAST_USED_INVALID);

        assertTrue(lastUsedTime != WebappDataStorage.LAST_USED_UNSET);
        assertTrue(lastUsedTime != WebappDataStorage.LAST_USED_INVALID);

        // Mark as launched, check launched recently.
        mSharedPreferences.edit()
                .putBoolean(WebappDataStorage.KEY_LAUNCHED, true)
                .commit();
        assertTrue(storage.wasLaunchedRecently());

        // Move the last used time one day in the past.
        mSharedPreferences.edit()
                .putLong(WebappDataStorage.KEY_LAST_USED, lastUsedTime - TimeUnit.DAYS.toMillis(1L))
                .commit();
        assertTrue(storage.wasLaunchedRecently());

        // Move the last used time three days in the past.
        mSharedPreferences.edit()
                .putLong(WebappDataStorage.KEY_LAST_USED, lastUsedTime - TimeUnit.DAYS.toMillis(3L))
                .commit();
        assertTrue(storage.wasLaunchedRecently());

        // Move the last used time one week in the past.
        mSharedPreferences.edit()
                .putLong(WebappDataStorage.KEY_LAST_USED, lastUsedTime - TimeUnit.DAYS.toMillis(7L))
                .commit();
        assertTrue(storage.wasLaunchedRecently());

        // Move the last used time just under ten days in the past.
        mSharedPreferences.edit().putLong(WebappDataStorage.KEY_LAST_USED,
                lastUsedTime - TimeUnit.DAYS.toMillis(10L) + 1).commit();
        assertTrue(storage.wasLaunchedRecently());

        // Mark as not launched.
        mSharedPreferences.edit()
                .putBoolean(WebappDataStorage.KEY_LAUNCHED, false)
                .commit();
        assertTrue(!storage.wasLaunchedRecently());

        // Mark as launched.
        mSharedPreferences.edit()
                .putBoolean(WebappDataStorage.KEY_LAUNCHED, true)
                .commit();
        assertTrue(storage.wasLaunchedRecently());

        // Move the last used time to exactly ten days in the past.
        mSharedPreferences.edit().putLong(WebappDataStorage.KEY_LAST_USED,
                lastUsedTime - TimeUnit.DAYS.toMillis(10L)).commit();
        assertTrue(!storage.wasLaunchedRecently());
    }

    @Test
    @Feature({"Webapp"})
    public void testIntentUpdate() throws Exception {
        final String id = "id";
        final String action = "action";
        final String url = "url";
        final String scope = "scope";
        final String name = "name";
        final String shortName = "shortName";
        final Bitmap icon = createBitmap();
        final int orientation = 1;
        final long themeColor = 2;
        final long backgroundColor = 3;
        final boolean isIconGenerated = false;
        Intent shortcutIntent = ShortcutHelper.createWebappShortcutIntent(id, action, url, scope,
                name, shortName, icon, ShortcutHelper.WEBAPP_SHORTCUT_VERSION, orientation,
                themeColor, backgroundColor, isIconGenerated);

        WebappDataStorage storage = WebappDataStorage.open(Robolectric.application, "test");
        storage.updateFromShortcutIntent(shortcutIntent);
        BackgroundShadowAsyncTask.runBackgroundTasks();
        Robolectric.runUiThreadTasks();

        assertEquals(action, mSharedPreferences.getString(WebappDataStorage.KEY_ACTION, null));
        assertEquals(url, mSharedPreferences.getString(WebappDataStorage.KEY_URL, null));
        assertEquals(scope, mSharedPreferences.getString(WebappDataStorage.KEY_SCOPE, null));
        assertEquals(name, mSharedPreferences.getString(WebappDataStorage.KEY_NAME, null));
        assertEquals(shortName,
                mSharedPreferences.getString(WebappDataStorage.KEY_SHORT_NAME, null));
        assertEquals(ShortcutHelper.encodeBitmapAsString(icon),
                mSharedPreferences.getString(WebappDataStorage.KEY_ICON, null));
        assertEquals(ShortcutHelper.WEBAPP_SHORTCUT_VERSION,
                mSharedPreferences.getInt(WebappDataStorage.KEY_VERSION, 0));
        assertEquals(orientation, mSharedPreferences.getInt(WebappDataStorage.KEY_ORIENTATION, 0));
        assertEquals(themeColor, mSharedPreferences.getLong(WebappDataStorage.KEY_THEME_COLOR, 0));
        assertEquals(backgroundColor,
                mSharedPreferences.getLong(WebappDataStorage.KEY_BACKGROUND_COLOR, 0));
        assertEquals(isIconGenerated,
                mSharedPreferences.getBoolean(WebappDataStorage.KEY_IS_ICON_GENERATED, true));

        // Wipe out the data and ensure that it is all gone.
        mSharedPreferences.edit().remove(WebappDataStorage.KEY_ACTION)
                .remove(WebappDataStorage.KEY_URL)
                .remove(WebappDataStorage.KEY_SCOPE)
                .remove(WebappDataStorage.KEY_NAME)
                .remove(WebappDataStorage.KEY_SHORT_NAME)
                .remove(WebappDataStorage.KEY_ICON)
                .remove(WebappDataStorage.KEY_VERSION)
                .remove(WebappDataStorage.KEY_ORIENTATION)
                .remove(WebappDataStorage.KEY_THEME_COLOR)
                .remove(WebappDataStorage.KEY_BACKGROUND_COLOR)
                .remove(WebappDataStorage.KEY_IS_ICON_GENERATED)
                .commit();

        assertEquals(null, mSharedPreferences.getString(WebappDataStorage.KEY_ACTION, null));
        assertEquals(null, mSharedPreferences.getString(WebappDataStorage.KEY_URL, null));
        assertEquals(null, mSharedPreferences.getString(WebappDataStorage.KEY_SCOPE, null));
        assertEquals(null, mSharedPreferences.getString(WebappDataStorage.KEY_NAME, null));
        assertEquals(null, mSharedPreferences.getString(WebappDataStorage.KEY_SHORT_NAME, null));
        assertEquals(null, mSharedPreferences.getString(WebappDataStorage.KEY_ICON, null));
        assertEquals(0, mSharedPreferences.getInt(WebappDataStorage.KEY_VERSION, 0));
        assertEquals(0, mSharedPreferences.getInt(WebappDataStorage.KEY_ORIENTATION, 0));
        assertEquals(0, mSharedPreferences.getLong(WebappDataStorage.KEY_THEME_COLOR, 0));
        assertEquals(0, mSharedPreferences.getLong(WebappDataStorage.KEY_BACKGROUND_COLOR, 0));
        assertEquals(true,
                mSharedPreferences.getBoolean(WebappDataStorage.KEY_IS_ICON_GENERATED, true));

        // Update again from the intent and ensure that the data is restored.
        storage.updateFromShortcutIntent(shortcutIntent);
        BackgroundShadowAsyncTask.runBackgroundTasks();
        Robolectric.runUiThreadTasks();

        assertEquals(action, mSharedPreferences.getString(WebappDataStorage.KEY_ACTION, null));
        assertEquals(url, mSharedPreferences.getString(WebappDataStorage.KEY_URL, null));
        assertEquals(scope, mSharedPreferences.getString(WebappDataStorage.KEY_SCOPE, null));
        assertEquals(name, mSharedPreferences.getString(WebappDataStorage.KEY_NAME, null));
        assertEquals(shortName,
                mSharedPreferences.getString(WebappDataStorage.KEY_SHORT_NAME, null));
        assertEquals(ShortcutHelper.encodeBitmapAsString(icon),
                mSharedPreferences.getString(WebappDataStorage.KEY_ICON, null));
        assertEquals(ShortcutHelper.WEBAPP_SHORTCUT_VERSION,
                mSharedPreferences.getInt(WebappDataStorage.KEY_VERSION, 0));
        assertEquals(orientation, mSharedPreferences.getInt(WebappDataStorage.KEY_ORIENTATION, 0));
        assertEquals(themeColor, mSharedPreferences.getLong(WebappDataStorage.KEY_THEME_COLOR, 0));
        assertEquals(backgroundColor,
                mSharedPreferences.getLong(WebappDataStorage.KEY_BACKGROUND_COLOR, 0));
        assertEquals(isIconGenerated,
                mSharedPreferences.getBoolean(WebappDataStorage.KEY_IS_ICON_GENERATED, true));
    }

    // TODO(lalitm) - There seems to be a bug in Robolectric where a Bitmap
    // produced from a byte stream is hardcoded to be a 100x100 bitmap with
    // ARGB_8888 pixel format. Because of this, we need to work around the
    // equality check of bitmaps. Remove this once the bug is fixed.
    private static boolean bitmapEquals(Bitmap expected, Bitmap actual) {
        if (actual.getWidth() != 100) return false;
        if (actual.getHeight() != 100) return false;
        if (!actual.getConfig().equals(Bitmap.Config.ARGB_8888)) return false;

        for (int i = 0; i < actual.getWidth(); i++) {
            for (int j = 0; j < actual.getHeight(); j++) {
                if (actual.getPixel(i, j) != 0) return false;
            }
        }
        return true;
    }

    private static Bitmap createBitmap() {
        return Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_4444);
    }
}
