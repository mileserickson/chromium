// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// loadTimeData contains localized content.  It is populated with
// file_manager_strings.grpd and chromeos_strings.grdp during build.

loadTimeData.data = $GRDP;

// Extend with additional fields not found in grdp files.
Object.setPrototypeOf(loadTimeData.data_, {
  'HIDE_SPACE_INFO': false,
  'UI_LOCALE': 'en_US',
  'language': 'en-US',
  'textdirection': 'ltr',
});

// Overwrite LoadTimeData.prototype.data setter as nop.
// Default implementation throws errors when both background and
// foreground re-set loadTimeData.data.
Object.defineProperty(LoadTimeData.prototype, 'data', {set: () => {}});
