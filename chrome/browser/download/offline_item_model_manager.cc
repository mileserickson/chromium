// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/offline_item_model_manager.h"

OfflineItemModelManager::OfflineItemModelManager() = default;

OfflineItemModelManager::~OfflineItemModelManager() = default;

OfflineItemModelData* OfflineItemModelManager::GetOrCreateOfflineItemModelData(
    const ContentId& id) {
  auto it = offline_item_model_data_.find(id);
  if (it != offline_item_model_data_.end())
    return it->second.get();
  offline_item_model_data_[id] = std::make_unique<OfflineItemModelData>();
  return offline_item_model_data_[id].get();
}

void OfflineItemModelManager::RemoveOfflineItemModelData(const ContentId& id) {
  offline_item_model_data_.erase(id);
}
