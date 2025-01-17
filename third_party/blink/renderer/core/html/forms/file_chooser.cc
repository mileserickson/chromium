/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/file_chooser.h"

#include "third_party/blink/public/web/web_local_frame_client.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/page/chrome_client_impl.h"
#include "third_party/blink/renderer/platform/wtf/date_math.h"

namespace blink {

FileChooserClient::~FileChooserClient() = default;

FileChooser* FileChooserClient::NewFileChooser(
    const WebFileChooserParams& params) {
  if (chooser_)
    chooser_->DisconnectClient();

  chooser_ = FileChooser::Create(this, params);
  return chooser_.get();
}

void FileChooserClient::DisconnectFileChooser() {
  DCHECK(HasConnectedFileChooser());
  chooser_->DisconnectClient();
}

inline FileChooser::FileChooser(FileChooserClient* client,
                                const WebFileChooserParams& params)
    : client_(client), params_(params) {}

scoped_refptr<FileChooser> FileChooser::Create(
    FileChooserClient* client,
    const WebFileChooserParams& params) {
  return base::AdoptRef(new FileChooser(client, params));
}

FileChooser::~FileChooser() = default;

bool FileChooser::OpenFileChooser(ChromeClientImpl& chrome_client_impl) {
  LocalFrame* frame = FrameOrNull();
  if (!frame)
    return false;
  chrome_client_impl_ = chrome_client_impl;

  WebLocalFrameClient* client =
      frame ? WebLocalFrameImpl::FromFrame(frame)->Client() : nullptr;
  if (!client || !client->RunFileChooser(params_, this))
    return false;

  // Should be released on file choosing.
  AddRef();
  chrome_client_impl.RegisterPopupOpeningObserver(client_);
  return true;
}

void FileChooser::DidChooseFile(const WebVector<WebString>& file_names) {
  FileChooserFileInfoList file_info;
  for (size_t i = 0; i < file_names.size(); ++i)
    file_info.push_back(FileChooserFileInfo(file_names[i]));
  ChooseFiles(file_info);
}

void FileChooser::DidChooseFile(const WebVector<SelectedFileInfo>& files) {
  FileChooserFileInfoList file_info;
  for (size_t i = 0; i < files.size(); ++i) {
    if (files[i].file_system_url.IsEmpty()) {
      file_info.push_back(
          FileChooserFileInfo(files[i].path, files[i].display_name));
    } else {
      FileMetadata metadata;
      metadata.modification_time = files[i].modification_time * kMsPerSecond;
      metadata.length = files[i].length;
      metadata.type = files[i].is_directory ? FileMetadata::kTypeDirectory
                                            : FileMetadata::kTypeFile;
      file_info.push_back(
          FileChooserFileInfo(files[i].file_system_url, metadata));
    }
  }
  ChooseFiles(file_info);
}

void FileChooser::ChooseFiles(const FileChooserFileInfoList& files) {
  // FIXME: This is inelegant. We should not be looking at params_ here.
  if (params_.selected_files.size() == files.size()) {
    bool was_changed = false;
    for (unsigned i = 0; i < files.size(); ++i) {
      if (String(params_.selected_files[i]) != files[i].path) {
        was_changed = true;
        break;
      }
    }
    if (!was_changed) {
      DidCloseChooser();
      return;
    }
  }

  if (client_)
    client_->FilesChosen(files);
  DidCloseChooser();
}

void FileChooser::DidCloseChooser() {
  if (chrome_client_impl_) {
    chrome_client_impl_->DidCompleteFileChooser(*this);
    if (client_)
      chrome_client_impl_->UnregisterPopupOpeningObserver(client_);
  }
  Release();
}

}  // namespace blink
