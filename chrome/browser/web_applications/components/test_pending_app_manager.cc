// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/components/test_pending_app_manager.h"

#include <utility>

#include "base/callback.h"
#include "chrome/browser/web_applications/components/install_result_code.h"
#include "url/gurl.h"

namespace web_app {

TestPendingAppManager::TestPendingAppManager() = default;

TestPendingAppManager::~TestPendingAppManager() = default;

void TestPendingAppManager::Install(AppInfo app_to_install,
                                    OnceInstallCallback callback) {
  // TODO(nigeltao): Add error simulation when error codes are added to the API.

  const GURL url(app_to_install.url);
  installed_apps_.push_back(std::move(app_to_install));
  std::move(callback).Run(url, InstallResultCode::kSuccess);
}

void TestPendingAppManager::InstallApps(
    std::vector<AppInfo> apps_to_install,
    const RepeatingInstallCallback& callback) {
  for (auto& app : apps_to_install)
    Install(std::move(app), callback);
}

void TestPendingAppManager::UninstallApps(std::vector<GURL> apps_to_uninstall,
                                          const UninstallCallback& callback) {
  for (auto& app : apps_to_uninstall) {
    const GURL url(app);
    uninstalled_apps_.push_back(std::move(app));
    callback.Run(url, true /* succeeded */);
  }
}

}  // namespace web_app
