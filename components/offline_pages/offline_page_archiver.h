// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_OFFLINE_PAGE_ARCHIVER_H_
#define COMPONENTS_OFFLINE_PAGES_OFFLINE_PAGE_ARCHIVER_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/strings/string16.h"
#include "url/gurl.h"

namespace offline_pages {

// Interface of a class responsible for creation of the archive for offline use.
//
// Archiver will be implemented by embedder and may have additional methods that
// are not interesting from the perspective of OfflinePageModel. Example of such
// extra information or capability is a way to enumerate available WebContents
// to find the one that needs to be used to create archive (or to map it to the
// URL passed in CreateArchive in some other way).
//
// Archiver will be responsible for naming the file that is being saved (it has
// URL, title and the whole page content at its disposal). For that it should be
// also configured with the path where the archives are stored.
//
// Archiver should be able to archive multiple pages in parallel, as these are
// asynchronous calls carried out by some other component.
//
// If archiver gets two consecutive requests to archive the same page (may be
// run in parallel) it can generate 2 different names for files and save the
// same page separately, as if these were 2 completely unrelated pages. It is up
// to the caller (e.g. OfflinePageModel) to make sure that situation like that
// does not happen.
//
// If the page is not completely loaded, it is up to the implementation of the
// archiver whether to respond with ERROR_CONTENT_UNAVAILBLE, wait longer to
// actually snapshot a complete page, or snapshot whatever is available at that
// point in time (what the user sees).
//
// TODO(fgorski): Add ability to delete archive.
// TODO(fgorski): Add ability to check that archive exists.
// TODO(fgorski): Add ability to refresh an existing archive in one step.
// TODO(fgorski): Add ability to identify all of the archives in the directory,
// to enable to model to reconcile the archives.
class OfflinePageArchiver {
 public:
  // Results of the archive creation.
  enum class ArchiverResult {
    SUCCESSFULLY_CREATED,           // Archive created successfully.
    ERROR_DEVICE_FULL,              // Cannot save the archive - device is full.
    ERROR_CANCELED,                 // Caller canceled the request.
    ERROR_CONTENT_UNAVAILABLE,      // Content to archive is not available.
    ERROR_ARCHIVE_CREATION_FAILED,  // Creation of archive failed.
  };

  typedef base::Callback<void(OfflinePageArchiver* /* archiver */,
                              ArchiverResult /* result */,
                              const GURL& /* url */,
                              const base::FilePath& /* file_path */,
                              int64_t /* file_size */)> CreateArchiveCallback;

  virtual ~OfflinePageArchiver() {}

  // Starts creating the archive in the |archives_dir| with |archive_id| added
  // to the archive filename. Once archive is created |callback| will be called
  // with the result and additional information.
  virtual void CreateArchive(const base::FilePath& archives_dir,
                             int64_t archive_id,
                             const CreateArchiveCallback& callback) = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_OFFLINE_PAGE_ARCHIVER_H_
