// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_
#define COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "components/cronet/cronet_url_request.h"
#include "components/cronet/cronet_url_request_context.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

namespace cronet {

class Cronet_EngineImpl;
class Cronet_UploadDataSinkImpl;

// Implementation of Cronet_UrlRequest that uses CronetURLRequestContext.
class Cronet_UrlRequestImpl : public Cronet_UrlRequest {
 public:
  Cronet_UrlRequestImpl();
  ~Cronet_UrlRequestImpl() override;

  // Cronet_UrlRequest
  Cronet_RESULT InitWithParams(Cronet_EnginePtr engine,
                               Cronet_String url,
                               Cronet_UrlRequestParamsPtr params,
                               Cronet_UrlRequestCallbackPtr callback,
                               Cronet_ExecutorPtr executor) override;
  Cronet_RESULT Start() override;
  Cronet_RESULT FollowRedirect() override;
  Cronet_RESULT Read(Cronet_BufferPtr buffer) override;
  void Cancel() override;
  bool IsDone() override;
  void GetStatus(Cronet_UrlRequestStatusListenerPtr listener) override;

  // Upload data provider has reported error while reading or rewinding
  // so request must fail.
  void OnUploadDataProviderError(const std::string& error_message);

 private:
  class NetworkTasks;

  // Return |true| if request has started and is now done.
  // Must be called under |lock_| held.
  bool IsDoneLocked();

  // Helper method to set final status of CronetUrlRequest and clean up the
  // native request adapter. Returns true if request is already done, false
  // request is not done and is destroyed.
  bool DestroyRequestUnlessDone(
      Cronet_RequestFinishedInfo_FINISHED_REASON finished_reason);

  // Helper method to set final status of CronetUrlRequest and clean up the
  // native request adapter. Returns true if request is already done, false
  // request is not done and is destroyed. Must be called under |lock_| held.
  bool DestroyRequestUnlessDoneLocked(
      Cronet_RequestFinishedInfo_FINISHED_REASON finished_reason);

  // Helper method to post |task| to the |executor_|.
  void PostTaskToExecutor(base::OnceClosure task);

  // Helper methods to invoke application |callback_|.
  void InvokeCallbackOnRedirectReceived();
  void InvokeCallbackOnResponseStarted();
  void InvokeCallbackOnReadCompleted(
      std::unique_ptr<Cronet_Buffer> cronet_buffer,
      int bytes_read);
  void InvokeCallbackOnSucceeded();
  void InvokeCallbackOnFailed();
  void InvokeCallbackOnCanceled();

  // Synchronize access to |request_| and other objects below from different
  // threads.
  base::Lock lock_;
  // Cronet URLRequest used for this operation.
  CronetURLRequest* request_ = nullptr;
  bool started_ = false;
  bool waiting_on_redirect_ = false;
  bool waiting_on_read_ = false;

  // Response info updated by callback with number of bytes received. May be
  // nullptr, if no response has been received.
  std::unique_ptr<Cronet_UrlResponseInfo> response_info_;
  // The error reported by request. May be nullptr if no error has occurred.
  std::unique_ptr<Cronet_Error> error_;

  // The upload data stream if specified.
  std::unique_ptr<Cronet_UploadDataSinkImpl> upload_data_sink_;

  // Application callback interface, used, but not owned, by |this|.
  Cronet_UrlRequestCallbackPtr callback_ = nullptr;
  // Executor for application callback, used, but not owned, by |this|.
  Cronet_ExecutorPtr executor_ = nullptr;

  // Cronet Engine used to run network operations. Not owned, accessed from
  // client thread. Must outlive this request.
  Cronet_EngineImpl* engine_;

#if DCHECK_IS_ON()
  // Event indicating Executor is properly destroying Runnables.
  base::WaitableEvent runnable_destroyed_;
#endif  // DCHECK_IS_ON()

  DISALLOW_COPY_AND_ASSIGN(Cronet_UrlRequestImpl);
};

};  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_URL_REQUEST_H_
