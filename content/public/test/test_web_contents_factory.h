// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_WEB_CONTENTS_FACTORY_H_
#define CONTENT_PUBLIC_TEST_TEST_WEB_CONTENTS_FACTORY_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"

namespace content {
class BrowserContext;
class RenderViewHostTestEnabler;
class WebContents;

// A helper class to create test web contents (tabs) for unit tests, without
// inheriting from RenderViewTestHarness. Can create web contents, and will
// clean up after itself upon destruction. Owns all created web contents.
// A few notes:
// - Works well allocated on the stack, because it should be destroyed before
//   associated browser context.
// - Doesn't play nice with web contents created any other way (because of
//   the implementation of RenderViewHostTestEnabler). But if you are creating
//   web contents already, what do you need this for? ;)
// TODO(devlin): The API is currently a bit sparse; there may need to be methods
// to, e.g., delete/close a web contents, access existing web contents, etc.
// These can be added as-needed.
class TestWebContentsFactory {
 public:
  TestWebContentsFactory();
  ~TestWebContentsFactory();

  // Creates a new WebContents with the given |context|, and returns it.
  // Ownership remains with the TestWebContentsFactory.
  WebContents* CreateWebContents(BrowserContext* context);

 private:
  // The test factory (and friends) for creating test web contents.
  scoped_ptr<RenderViewHostTestEnabler> rvh_enabler_;

  // The vector of web contents that this class created.
  ScopedVector<WebContents> web_contents_;

  DISALLOW_COPY_AND_ASSIGN(TestWebContentsFactory);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_WEB_CONTENTS_FACTORY_H_
