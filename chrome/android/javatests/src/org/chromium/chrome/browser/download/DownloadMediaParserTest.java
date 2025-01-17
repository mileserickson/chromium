// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.support.test.filters.LargeTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.content_public.browser.test.util.Criteria;
import org.chromium.content_public.browser.test.util.CriteriaHelper;

import java.io.File;

/**
 * Tests to verify DownloadMediaParser, which retrieves media metadata and thumbnails.
 *
 * Most of the work is done in utility process and GPU process.
 *
 * All download media parser usage must be called on UI thread in this test to get message loop and
 * threading contexts in native.
 *
 * Because each media parser call may perform multiple process and thread hops, it can be slow.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class DownloadMediaParserTest {
    private static final long MAX_MEDIA_PARSER_POLL_TIME_MS = 10000;
    private static final long MEDIA_PARSER_POLL_INTERVAL_MS = 1000;

    @Rule
    public ChromeBrowserTestRule mTestRule = new ChromeBrowserTestRule();

    /**
     * Wraps result from download media parser.
     */
    public static class MediaParseResult {
        public boolean done;
        public DownloadMediaData mediaData;
    }

    @Before
    public void setUp() throws Exception {
        mTestRule.loadNativeLibraryAndInitBrowserProcess();
    }

    @Test
    @LargeTest
    @Feature({"Download"})
    @RetryOnFailure
    public void testParseAudioMetatadata() throws InterruptedException {
        String filePath = UrlUtils.getIsolatedTestRoot() + "/media/test/data/sfx.mp3";
        File audioFile = new File(filePath);
        Assert.assertTrue(audioFile.exists());
        boolean done = false;
        MediaParseResult result = new MediaParseResult();

        // The native DownloadMediaParser needs to be created on UI thread.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            DownloadMediaParserBridge parser = new DownloadMediaParserBridge(
                    "audio/mp3", filePath, audioFile.length(), (DownloadMediaData mediaData) -> {
                        result.mediaData = mediaData;
                        result.done = true;
                    });
            parser.start();
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return result.done;
            }
        }, MAX_MEDIA_PARSER_POLL_TIME_MS, MEDIA_PARSER_POLL_INTERVAL_MS);

        Assert.assertTrue("Failed to parse audio metadata.", result.mediaData != null);
    }

    @Test
    @LargeTest
    @Feature({"Download"})
    @RetryOnFailure
    public void testParseVideoMetatadataThumbnail() throws InterruptedException {
        String filePath = UrlUtils.getIsolatedTestRoot() + "/media/test/data/bear.mp4";
        File videoFile = new File(filePath);
        Assert.assertTrue(videoFile.exists());
        boolean done = false;
        MediaParseResult result = new MediaParseResult();

        // The native DownloadMediaParser needs to be created on UI thread.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            DownloadMediaParserBridge parser = new DownloadMediaParserBridge(
                    "video/mp4", filePath, videoFile.length(), (DownloadMediaData mediaData) -> {
                        result.mediaData = mediaData;
                        result.done = true;
                    });
            parser.start();
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return result.done;
            }
        }, MAX_MEDIA_PARSER_POLL_TIME_MS, MEDIA_PARSER_POLL_INTERVAL_MS);

        Assert.assertTrue("Failed to parse video file.", result.mediaData != null);
        Assert.assertTrue(
                "Failed to retrieve thumbnail.", result.mediaData.thumbnail.getWidth() > 0);
        Assert.assertTrue(
                "Failed to retrieve thumbnail.", result.mediaData.thumbnail.getHeight() > 0);
    }
}
