// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/public/cpp/window_properties.h"
#include "chrome/browser/apps/platform_apps/app_browsertest_util.h"
#include "chrome/browser/ui/ash/tablet_mode_client.h"
#include "chrome/browser/ui/ash/tablet_mode_client_test_util.h"
#include "chrome/browser/ui/views/apps/chrome_native_app_window_views_aura_ash.h"
#include "chromeos/login/login_state.h"
#include "chromeos/login/scoped_test_public_session_login_state.h"
#include "extensions/browser/app_window/app_window.h"
#include "ui/aura/window.h"
#include "ui/base/ui_base_types.h"
#include "ui/wm/core/window_util.h"

class ChromeNativeAppWindowViewsAuraAshBrowserTest
    : public extensions::PlatformAppBrowserTest {
 public:
  ChromeNativeAppWindowViewsAuraAshBrowserTest() = default;
  ~ChromeNativeAppWindowViewsAuraAshBrowserTest() override = default;

 protected:
  void InitWindow() { app_window_ = CreateTestAppWindow("{}"); }

  bool IsImmersiveActive() {
    return window()->widget()->GetNativeWindow()->GetProperty(
        ash::kImmersiveIsActive);
  }

  ChromeNativeAppWindowViewsAuraAsh* window() {
    return static_cast<ChromeNativeAppWindowViewsAuraAsh*>(
        GetNativeAppWindowForAppWindow(app_window_));
  }

  extensions::AppWindow* app_window_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeNativeAppWindowViewsAuraAshBrowserTest);
};

// Verify that immersive mode is enabled or disabled as expected.
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       ImmersiveWorkFlow) {
  InitWindow();
  ASSERT_TRUE(window());
  EXPECT_FALSE(IsImmersiveActive());
  constexpr int kFrameHeight = 32;

  views::ClientView* client_view =
      window()->widget()->non_client_view()->client_view();
  EXPECT_EQ(kFrameHeight, client_view->bounds().y());

  // Verify that when fullscreen is toggled on, immersive mode is enabled and
  // that when fullscreen is toggled off, immersive mode is disabled.
  app_window_->OSFullscreen();
  EXPECT_TRUE(IsImmersiveActive());
  EXPECT_EQ(0, client_view->bounds().y());

  app_window_->Restore();
  EXPECT_FALSE(IsImmersiveActive());
  EXPECT_EQ(kFrameHeight, client_view->bounds().y());

  // Verify that since the auto hide title bars in tablet mode feature turned
  // on, immersive mode is enabled once tablet mode is entered, and disabled
  // once tablet mode is exited.
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(true));
  EXPECT_TRUE(IsImmersiveActive());
  EXPECT_EQ(0, client_view->bounds().y());

  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(false));
  EXPECT_FALSE(IsImmersiveActive());
  EXPECT_EQ(kFrameHeight, client_view->bounds().y());

  // Verify that the window was fullscreened before entering tablet mode, it
  // will remain fullscreened after exiting tablet mode.
  app_window_->OSFullscreen();
  EXPECT_TRUE(IsImmersiveActive());
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(true));
  EXPECT_TRUE(IsImmersiveActive());
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(false));
  EXPECT_TRUE(IsImmersiveActive());
  app_window_->Restore();

  // Verify that minimized windows do not have immersive mode enabled.
  app_window_->Minimize();
  EXPECT_FALSE(IsImmersiveActive());
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(true));
  EXPECT_FALSE(IsImmersiveActive());
  window()->Restore();
  EXPECT_TRUE(IsImmersiveActive());
  app_window_->Minimize();
  EXPECT_FALSE(IsImmersiveActive());
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(false));
  EXPECT_FALSE(IsImmersiveActive());

  // Verify that activation change should not change the immersive
  // state.
  window()->Show();
  app_window_->OSFullscreen();
  EXPECT_TRUE(IsImmersiveActive());
  ::wm::DeactivateWindow(window()->GetNativeWindow());
  EXPECT_TRUE(IsImmersiveActive());
  ::wm::ActivateWindow(window()->GetNativeWindow());
  EXPECT_TRUE(IsImmersiveActive());

  CloseAppWindow(app_window_);
}

// Verifies that apps in immersive fullscreen will have a restore state of
// maximized.
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       ImmersiveModeFullscreenRestoreType) {
  InitWindow();
  ASSERT_TRUE(window());

  app_window_->OSFullscreen();
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window()->GetRestoredState());
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(true));
  EXPECT_TRUE(window()->IsFullscreen());
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window()->GetRestoredState());
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(false));
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window()->GetRestoredState());

  CloseAppWindow(app_window_);
}

// Verify that immersive mode stays disabled when entering tablet mode in
// forced fullscreen mode (e.g. when running in a kiosk session).
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       NoImmersiveModeWhenForcedFullscreen) {
  InitWindow();
  ASSERT_TRUE(window());

  app_window_->ForcedFullscreen();

  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(true));
  EXPECT_FALSE(IsImmersiveActive());
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(false));
  EXPECT_FALSE(IsImmersiveActive());
}

// Make sure a normal window is not in immersive mode, and uses
// immersive in fullscreen.
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       PublicSessionImmersiveMode) {
  chromeos::ScopedTestPublicSessionLoginState login_state;

  InitWindow();
  ASSERT_TRUE(window());
  EXPECT_FALSE(IsImmersiveActive());

  app_window_->SetFullscreen(extensions::AppWindow::FULLSCREEN_TYPE_HTML_API,
                             true);

  EXPECT_TRUE(IsImmersiveActive());
}

// Verifies that apps in clamshell mode with immersive fullscreen enabled will
// correctly exit immersive mode if exit fullscreen.
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       RestoreImmersiveMode) {
  InitWindow();
  ASSERT_TRUE(window());

  // Should not disable immersive fullscreen in tablet mode if |window| exits
  // fullscreen.
  EXPECT_FALSE(window()->IsFullscreen());
  app_window_->OSFullscreen();
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window()->GetRestoredState());
  EXPECT_TRUE(window()->IsFullscreen());
  EXPECT_TRUE(IsImmersiveActive());
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(true));
  EXPECT_TRUE(window()->IsFullscreen());
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window()->GetRestoredState());

  window()->Restore();
  // Restoring a window inside tablet mode should deactivate fullscreen, but not
  // disable immersive mode.
  EXPECT_FALSE(window()->IsFullscreen());
  ASSERT_TRUE(IsImmersiveActive());

  // Immersive fullscreen should be disabled if window exits fullscreen in
  // clamshell mode.
  ASSERT_NO_FATAL_FAILURE(test::SetAndWaitForTabletMode(false));
  app_window_->OSFullscreen();
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window()->GetRestoredState());
  EXPECT_TRUE(window()->IsFullscreen());

  window()->Restore();
  EXPECT_FALSE(IsImmersiveActive());

  CloseAppWindow(app_window_);
}
