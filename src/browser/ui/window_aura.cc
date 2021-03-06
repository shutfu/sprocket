// Copyright (c) 2015 University of Szeged.
// Copyright (c) 2015 The Chromium Authors.
// All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sprocket/browser/ui/window.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "sprocket/browser/ui/authentication_dialog.h"
#include "sprocket/browser/ui/color_chooser.h"
#include "sprocket/browser/ui/context_menu_model.h"
#include "sprocket/browser/ui/javascript_dialog.h"
#include "sprocket/browser/ui/views_delegate_aura.h"
#include "sprocket/browser/ui/window_delegate_view_aura.h"
#include "sprocket/browser/ui/tab.h"
#include "sprocket/browser/ui/tabbed_pane.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window_tree_host.h"
#include "ui/views/color_chooser/color_chooser_view.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/views/widget/widget_delegate.h"

views::ViewsDelegate* SprocketWindow::views_delegate_ = NULL;

// static
void SprocketWindow::PlatformInitialize() {
  display::Screen::SetScreenInstance(views::CreateDesktopScreen());
  views_delegate_ = new SprocketViewsDelegateAura;
}

// static
void SprocketWindow::PlatformExit() {
  delete views_delegate_;
  views_delegate_ = NULL;
}

void SprocketWindow::PlatformCleanUp() {
}

void SprocketWindow::PlatformCreateWindow(int width, int height) {
  window_widget_ = new views::Widget;
  views::Widget::InitParams params;
  params.bounds = gfx::Rect(0, 0, width, height);
  params.delegate = new SprocketWindowDelegateView;
  window_widget_->Init(params);

  content_size_ = gfx::Size(width, height);
  window_ = window_widget_->GetNativeWindow();

  window_->GetHost()->Show();
  window_widget_->Show();
}

void SprocketWindow::PlatformCloseWindow() {
  window_widget_->CloseNow();
}

void SprocketWindow::PlatformAddTab(SprocketWebContents* sprocket_web_contents) {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  delegate_view->AddTab(sprocket_web_contents, content_size_);
}

void SprocketWindow::PlatformSelectTabAt(int index) {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  delegate_view->SelectTabAt(index);
}

Tab* SprocketWindow::PlatformGetSelectedTab() {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  return delegate_view->GetSelectedTab();
}

void SprocketWindow::PlatformSelectTab(Tab* tab) {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  delegate_view->SelectTab(tab);
}

void SprocketWindow::PlatformEnableUIControl(UIControl control, bool is_enabled) {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  if (control == BACK_BUTTON)
    delegate_view->EnableUIControl(BACK_BUTTON, is_enabled);
  else if (control == FORWARD_BUTTON)
    delegate_view->EnableUIControl(FORWARD_BUTTON, is_enabled);
  else if (control == REFRESH_STOP_BUTTON)
    delegate_view->EnableUIControl(REFRESH_STOP_BUTTON, is_enabled);
}

void SprocketWindow::PlatformSetAddressBarURL(const GURL& url) {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  delegate_view->SetAddressBarURL(url);
}

void SprocketWindow::PlatformSetIsLoading(bool loading) {
}

void SprocketWindow::PlatformSetTitle(const base::string16& title) {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  delegate_view->SetWindowTitle(title);
  window_widget_->UpdateWindowTitle();
}

bool SprocketWindow::PlatformHandleContextMenu(const content::ContextMenuParams& params) {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  delegate_view->ShowWebContentsContextMenu(params);
  return true;
}

void SprocketWindow::PlatformLoadProgressChanged(double progress) {
  // TODO: Implement!
}

void SprocketWindow::PlatformShowDialog(views::WidgetDelegate* delegate) {
  content::WebContents* web_contents =
      PlatformGetSelectedTab()->sprocket_web_contents()->web_contents();
  views::Widget* dialog_widget =
      views::DialogDelegate::CreateDialogWidget(
          delegate,
          window_,
          web_contents->GetNativeView());
  gfx::Rect bounds = window_widget_->GetWindowBoundsInScreen();
  bounds.ClampToCenteredSize(dialog_widget->GetWindowBoundsInScreen().size());
  dialog_widget->SetBounds(bounds);
  // TODO: This does not work: dialog_widget->SetAlwaysOnTop(true);
  dialog_widget->Show();
}

void SprocketWindow::PlatformShowJavaScriptDialog(SprocketJavaScriptDialog* dialog) {
  PlatformShowDialog(dialog);
}

void SprocketWindow::PlatformShowColorChooserDialog(SprocketColorChooser* listener,
                                                    SkColor initial_color) {
  views::ColorChooserView* color_chooser_view =
      new views::ColorChooserView(listener, initial_color);
  PlatformShowDialog(color_chooser_view);
}

void SprocketWindow::PlatformShowAuthenticationDialog(SprocketAuthenticationDialog* dialog) {
  PlatformShowDialog(dialog);
}

void SprocketWindow::PlatformToggleFullscreenModeForTab(bool enter_fullscreen) {
  was_fullscreen_ = is_fullscreen_;
  if (is_fullscreen_ != enter_fullscreen) {
    is_fullscreen_ = enter_fullscreen;
    window_widget_->SetFullscreen(enter_fullscreen);
    SprocketWindowDelegateView* delegate_view =
        static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
    delegate_view->SetFullscreen(enter_fullscreen);

    if (!is_fullscreen_) {
      SprocketWebContents* sprocket_web_contents = PlatformGetSelectedTab()->sprocket_web_contents();
      sprocket_web_contents->ExitFullscreenModeForTab(sprocket_web_contents->web_contents());
    }
  }
}

bool SprocketWindow::PlatformIsFullscreenForTabOrPending() const {
  return is_fullscreen_;
}

bool SprocketWindow::PlatformWasFullscreenForTab() const {
  return was_fullscreen_;
}

void SprocketWindow::PlatformHandleKeyboardEvent(const content::NativeWebKeyboardEvent& event) {
  SprocketWindowDelegateView* delegate_view =
      static_cast<SprocketWindowDelegateView*>(window_widget_->widget_delegate());
  delegate_view->HandleKeyboardEvent(event);
}
