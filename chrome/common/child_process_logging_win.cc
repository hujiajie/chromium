// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/child_process_logging.h"

#include <windows.h>

#include "base/debug/crash_logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/crash_keys.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/metrics/client_info.h"

namespace child_process_logging {

namespace {

// exported in breakpad_win.cc/crashpad_win.cc:
//    void __declspec(dllexport) __cdecl SetCrashKeyValueImpl.
typedef void(__cdecl* SetCrashKeyValue)(const wchar_t*, const wchar_t*);

// exported in breakpad_win.cc/crashpad_win.cc:
//    void __declspec(dllexport) __cdecl ClearCrashKeyValueImpl.
typedef void(__cdecl* ClearCrashKeyValue)(const wchar_t*);

void SetCrashKeyValueTrampoline(const base::StringPiece& key,
                                const base::StringPiece& value) {
  static SetCrashKeyValue set_crash_key = [](){
    HMODULE exe_module = GetModuleHandle(chrome::kBrowserProcessExecutableName);
    return reinterpret_cast<SetCrashKeyValue>(
        exe_module ? GetProcAddress(exe_module, "SetCrashKeyValueImpl")
                   : nullptr);
  }();
  if (set_crash_key) {
    (set_crash_key)(base::UTF8ToWide(key).data(),
                    base::UTF8ToWide(value).data());
  }
}

void ClearCrashKeyValueTrampoline(const base::StringPiece& key) {
  static ClearCrashKeyValue clear_crash_key = [](){
    HMODULE exe_module = GetModuleHandle(chrome::kBrowserProcessExecutableName);
    return reinterpret_cast<ClearCrashKeyValue>(
        exe_module ? GetProcAddress(exe_module, "ClearCrashKeyValueImpl")
                   : nullptr);
  }();
  if (clear_crash_key)
    (clear_crash_key)(base::UTF8ToWide(key).data());
}

}  // namespace

void Init() {
  crash_keys::RegisterChromeCrashKeys();
  base::debug::SetCrashKeyReportingFunctions(&SetCrashKeyValueTrampoline,
                                             &ClearCrashKeyValueTrampoline);

  // This would be handled by BreakpadClient::SetCrashClientIdFromGUID(), but
  // because of the aforementioned issue, crash keys aren't ready yet at the
  // time of Breakpad initialization, load the client id backed up in Google
  // Update settings instead.
  scoped_ptr<metrics::ClientInfo> client_info =
      GoogleUpdateSettings::LoadMetricsClientInfo();
  if (client_info)
    crash_keys::SetMetricsClientIdFromGUID(client_info->client_id);
}

}  // namespace child_process_logging
