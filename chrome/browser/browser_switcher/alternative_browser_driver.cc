// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_switcher/alternative_browser_driver.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/process/launch.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "chrome/browser/browser_switcher/alternative_browser_launcher.h"
#include "url/gurl.h"

#include <ddeml.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windows.h>
#include <wininet.h>

namespace browser_switcher {

namespace {

const wchar_t kUrlVarName[] = L"${url}";

const wchar_t kIExploreKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";
const wchar_t kFirefoxKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\firefox.exe";
// Opera does not register itself here for now but it's no harm to keep this.
const wchar_t kOperaKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\opera.exe";
const wchar_t kSafariKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\safari.exe";
const wchar_t kChromeKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe";

const wchar_t kIExploreDdeHost[] = L"IExplore";

const wchar_t kChromeVarName[] = L"${chrome}";
const wchar_t kIEVarName[] = L"${ie}";
const wchar_t kFirefoxVarName[] = L"${firefox}";
const wchar_t kOperaVarName[] = L"${opera}";
const wchar_t kSafariVarName[] = L"${safari}";

const struct {
  const wchar_t* var_name;
  const wchar_t* registry_key;
  const wchar_t* dde_host;
} kBrowserVarMappings[] = {
    {kChromeVarName, kChromeKey, L""},
    {kIEVarName, kIExploreKey, kIExploreDdeHost},
    {kFirefoxVarName, kFirefoxKey, L""},
    {kOperaVarName, kOperaKey, L""},
    {kSafariVarName, kSafariKey, L""},
};

// DDE Callback function which is not used in our case at all.
HDDEDATA CALLBACK DdeCallback(UINT type,
                              UINT format,
                              HCONV handle,
                              HSZ string1,
                              HSZ string2,
                              HDDEDATA data,
                              ULONG_PTR data1,
                              ULONG_PTR data2) {
  return NULL;
}

void PercentEncodeCommas(std::wstring* url) {
  size_t pos = url->find(L",");
  while (pos != std::wstring::npos) {
    url->replace(pos, 1, L"%2C");
    pos = url->find(L",", pos);
  }
}

std::wstring CompileCommandLine(const std::wstring& raw_command_line,
                                const GURL& url) {
  std::wstring url_spec = base::UTF8ToWide(GURL(url).spec());
  std::wstring command_line = raw_command_line;
  size_t pos = command_line.find(kUrlVarName);
  if (pos != command_line.npos) {
    command_line = command_line.replace(pos, wcslen(kUrlVarName), url_spec);
  } else {
    if (command_line.empty())
      command_line = url_spec;
    else
      command_line.append(L" ").append(url_spec);
  }
  return command_line;
}

std::wstring GetBrowserLocation(const wchar_t* regkey_name) {
  DCHECK(regkey_name);
  base::win::RegKey key;
  if (ERROR_SUCCESS != key.Open(HKEY_LOCAL_MACHINE, regkey_name, KEY_READ) &&
      ERROR_SUCCESS != key.Open(HKEY_CURRENT_USER, regkey_name, KEY_READ)) {
    LOG(ERROR) << "Could not open registry key " << regkey_name
               << "! Error Code:" << GetLastError();
    return std::wstring();
  }
  std::wstring location;
  if (ERROR_SUCCESS != key.ReadValue(NULL, &location))
    return std::wstring();
  return location;
}

}  // namespace

AlternativeBrowserDriver::~AlternativeBrowserDriver() {}

AlternativeBrowserDriverImpl::AlternativeBrowserDriverImpl() {}
AlternativeBrowserDriverImpl::~AlternativeBrowserDriverImpl() {}

void AlternativeBrowserDriverImpl::SetBrowserPath(base::StringPiece path) {
  browser_path_ = base::UTF8ToWide(path);
  dde_host_ = std::wstring();
  if (browser_path_.empty()) {
    browser_path_ = GetBrowserLocation(kIExploreKey);
    dde_host_ = kIExploreDdeHost;
    return;
  }
  for (const auto& mapping : kBrowserVarMappings) {
    if (!browser_path_.compare(mapping.var_name)) {
      browser_path_ = GetBrowserLocation(mapping.registry_key);
      dde_host_ = mapping.dde_host;
    }
  }
}

void AlternativeBrowserDriverImpl::SetBrowserParameters(
    base::StringPiece parameters) {
  browser_params_ = base::UTF8ToWide(parameters);
}

bool AlternativeBrowserDriverImpl::TryLaunch(const GURL& url) {
  VLOG(2) << "Launching alternative browser...";
  VLOG(2) << "  path = " << browser_path_;
  VLOG(2) << "  parameters = " << browser_params_;
  VLOG(2) << "  dde_host = " << dde_host_;
  VLOG(2) << "  url = " << url.spec();
  return (TryLaunchWithDde(url) || TryLaunchWithExec(url));
}

bool AlternativeBrowserDriverImpl::TryLaunchWithDde(const GURL& url) {
  if (dde_host_.empty())
    return false;

  DWORD dde_instance = 0;
  if (DdeInitialize(&dde_instance, DdeCallback, CBF_FAIL_ALLSVRXACTIONS, 0) !=
      DMLERR_NO_ERROR) {
    return false;
  }

  bool success = false;
  HCONV openurl_service_instance;
  HCONV activate_service_instance;
  {
    HSZ service =
        DdeCreateStringHandle(dde_instance, dde_host_.c_str(), CP_WINUNICODE);
    HSZ openurl_topic =
        DdeCreateStringHandle(dde_instance, L"WWW_OpenURL", CP_WINUNICODE);
    HSZ activate_topic =
        DdeCreateStringHandle(dde_instance, L"WWW_Activate", CP_WINUNICODE);
    openurl_service_instance =
        DdeConnect(dde_instance, service, openurl_topic, NULL);
    activate_service_instance =
        DdeConnect(dde_instance, service, activate_topic, NULL);
    DdeFreeStringHandle(dde_instance, service);
    DdeFreeStringHandle(dde_instance, openurl_topic);
    DdeFreeStringHandle(dde_instance, activate_topic);
  }

  if (openurl_service_instance) {
    // Percent-encode commas and spaces because those mean something else
    // for the WWW_OpenURL verb and the url is trimmed on the first one.
    // Spaces are already encoded by GURL.
    std::wstring encoded_url(base::UTF8ToWide(url.spec()));
    PercentEncodeCommas(&encoded_url);

    success =
        DdeClientTransaction(
            reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(encoded_url.data())),
            encoded_url.size() * sizeof(wchar_t), openurl_service_instance, 0,
            0, XTYP_EXECUTE, TIMEOUT_ASYNC, NULL) != 0;
    DdeDisconnect(openurl_service_instance);
    if (activate_service_instance) {
      if (success) {
        // Bring window to the front.
        wchar_t cmd[] = L"0xFFFFFFFF,0x0";
        DdeClientTransaction(reinterpret_cast<LPBYTE>(cmd), sizeof(cmd),
                             activate_service_instance, 0, 0, XTYP_EXECUTE,
                             TIMEOUT_ASYNC, NULL);
      }
      DdeDisconnect(activate_service_instance);
    }
  }
  DdeUninitialize(dde_instance);
  return success;
}

bool AlternativeBrowserDriverImpl::TryLaunchWithExec(const GURL& url) {
  base::CommandLine cmd_line = base::CommandLine(base::FilePath(browser_path_));
  cmd_line.AppendArgNative(
      base::WideToUTF16(CompileCommandLine(browser_params_, url)));
  base::LaunchOptions options;
  if (!base::LaunchProcess(cmd_line, options).IsValid()) {
    LOG(ERROR) << "Could not start the alternative browser! Error: "
               << GetLastError();
    return false;
  }
  return true;
}

}  // namespace browser_switcher
