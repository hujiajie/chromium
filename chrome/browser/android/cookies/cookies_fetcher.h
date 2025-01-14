// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_COOKIES_COOKIES_FETCHER_H_
#define CHROME_BROWSER_ANDROID_COOKIES_COOKIES_FETCHER_H_

#include <stdint.h>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "net/cookies/canonical_cookie.h"
#include "net/url_request/url_request_context_getter.h"

class Profile;

// This class can be used to retrieve an array of cookies from the cookie jar
// as well as insert an array of cookies into it. This class is the underlying
// glue that interacts with CookiesFetch.java and its lifetime is governed by
// the Java counter part.
class CookiesFetcher {
 public:
  // Constructs a fetcher that can interact with the cookie jar in the
  // specified profile.
  explicit CookiesFetcher(JNIEnv* env, jobject obj, Profile* profile);

  ~CookiesFetcher();

  // Called by the Java object when it is getting GC'd.
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  // Callback used after the cookie jar populate the cookie list for us.
  void OnCookiesFetchFinished(const net::CookieList& cookies);

  // Fetches all cookies from the cookie jar.
  void PersistCookies(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& obj);

  // Saves a cookie to the cookie jar.
  void RestoreCookies(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& obj,
                      const base::android::JavaParamRef<jstring>& url,
                      const base::android::JavaParamRef<jstring>& name,
                      const base::android::JavaParamRef<jstring>& value,
                      const base::android::JavaParamRef<jstring>& domain,
                      const base::android::JavaParamRef<jstring>& path,
                      int64_t creation,
                      int64_t expiration,
                      int64_t last_access,
                      bool secure,
                      bool httponly,
                      int samesite,
                      int priority);

 private:
  void PersistCookiesInternal(net::URLRequestContextGetter* getter);
  void RestoreToCookieJarInternal(net::URLRequestContextGetter* getter,
                                  const net::CanonicalCookie& cookie);

  base::android::ScopedJavaGlobalRef<jobject> jobject_;

  DISALLOW_COPY_AND_ASSIGN(CookiesFetcher);
};

// Registers the CookiesFetcher native method.
bool RegisterCookiesFetcher(JNIEnv* env);

#endif // CHROME_BROWSER_ANDROID_COOKIES_COOKIES_FETCHER_H_
