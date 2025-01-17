// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_STATUS_H_
#define COMPONENTS_INVALIDATION_IMPL_STATUS_H_

#include <string>

namespace syncer {

// Status of the message arrived from FCM.
// Used by UMA histogram, so entries shouldn't be reordered or removed.
enum class InvalidationParsingStatus {
  kSuccess = 0,
  kPublicTopicEmpty = 1,
  kPrivateTopicEmpty = 2,
  kVersionEmpty = 3,
  kMaxValue = kVersionEmpty,
};

// This enum indicates how an operation was completed. These values are written
// to logs.  New enum values can be added, but existing enums must never be
// renumbered or deleted and reused.
enum class StatusCode {
  // The operation has been completed successfully.
  SUCCESS = 0,
  // The operation failed.
  FAILED = 1
};

// This struct provides the status code of a request and an optional message
// describing the status (esp. failures) in detail.
struct Status {
  Status(StatusCode status_code, const std::string& message);
  ~Status();

  // Errors always need a message but a success does not.
  static Status Success();

  bool IsSuccess() const { return code == StatusCode::SUCCESS; }

  StatusCode code;
  // The message is not meant to be displayed to the user.
  std::string message;

  // Copy and assignment allowed.
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_STATUS_H_
