// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/blob/blob_utils.h"

#include <algorithm>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/common/features.h"

namespace blink {

// static
bool BlobUtils::MojoBlobURLsEnabled() {
  return base::FeatureList::IsEnabled(network::features::kNetworkService) ||
         base::FeatureList::IsEnabled(blink::features::kMojoBlobURLs);
}

namespace {

const base::Feature kBlobDataPipeTuningFeature{
    "BlobDataPipeTuning", base::FEATURE_DISABLED_BY_DEFAULT};

constexpr int kBlobMinDataPipeCapacity = 1024;
constexpr int kBlobDefaultDataPipeCapacity = 512 * 1024;

constexpr base::FeatureParam<int> kBlobDataPipeCapacity{
    &kBlobDataPipeTuningFeature, "capacity_bytes",
    kBlobDefaultDataPipeCapacity};

constexpr int kBlobMinDataPipeChunkSize = 1024;
constexpr int kBlobDefaultDataPipeChunkSize = 64 * 1024;

constexpr base::FeatureParam<int> kBlobDataPipeChunkSize{
    &kBlobDataPipeTuningFeature, "chunk_bytes", kBlobDefaultDataPipeChunkSize};

}  // namespace

// static
uint32_t BlobUtils::GetDataPipeCapacity() {
  return std::max(kBlobDataPipeCapacity.Get(), kBlobMinDataPipeCapacity);
}

// static
uint32_t BlobUtils::GetDataPipeChunkSize() {
  // The mojo::DataPipe will allow up to 64KB to be written into it in
  // a single chunk, but there may be some advantage to writing smaller
  // chunks.  For example, the network stack uses 32KB chunks.  This could
  // result in the faster delivery of the first byte of data when reading
  // from a slow disk.
  return std::max(kBlobDataPipeChunkSize.Get(), kBlobMinDataPipeChunkSize);
}

}  // namespace blink
