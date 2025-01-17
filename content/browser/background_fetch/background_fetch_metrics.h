// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_METRICS_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_METRICS_H_

#include "third_party/blink/public/platform/modules/background_fetch/background_fetch.mojom.h"

namespace content {

namespace background_fetch {

// Records the |error| status issued by the DataManager after it was requested
// to mark a Background Fetch registration for deletion. The marking is invoked
// by the scheduler controller after it is finished.
void RecordSchedulerFinishedError(blink::mojom::BackgroundFetchError error);

// Records the |error| status issued by the DataManager after it was requested
// to create and store a new Background Fetch registration.
void RecordRegistrationCreatedError(blink::mojom::BackgroundFetchError error);

// Records the |error| status issued by the DataManager after the storage
// associated with a registration has been completely deleted.
void RecordRegistrationDeletedError(blink::mojom::BackgroundFetchError error);

// Records the number of registrations that have unfinished fetches found on
// start-up.
void RecordRegistrationsOnStartup(int num_registrations);

// Records the BackgroundFetch UKM event. Must be called before a Background
// Fetch registration has been created. Will be a no-op if |frame_tree_node_id|
// does not identify a valid, live frame.
void RecordBackgroundFetchUkmEvent(
    const url::Origin& origin,
    const std::vector<ServiceWorkerFetchRequest>& requests,
    const BackgroundFetchOptions& options,
    const SkBitmap& icon,
    blink::mojom::BackgroundFetchUkmDataPtr ukm_data,
    int frame_tree_node_id,
    bool has_permission);

}  // namespace background_fetch

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_METRICS_H_
