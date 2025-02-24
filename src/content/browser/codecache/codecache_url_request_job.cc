// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "content/browser/codecache/codecache_url_request_job.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
#include "base/task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "net/code_cache/code_cache.h"
#include "net/base/file_stream.h"
#include "net/base/filename_util.h"
#include "net/base/io_buffer.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/filter/gzip_source_stream.h"
#include "net/filter/source_stream.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_file_dir_job.h"
#include "url/gurl.h"

#if defined(OS_WIN)
#include "base/win/shortcut.h"
#endif

namespace content {
using net::CodeCache;
using net::ERR_ACCESS_DENIED;
using net::ERR_FILE_NOT_FOUND;
using net::ERR_IO_PENDING;
using net::ERR_IO_PENDING;
using net::ERR_REQUEST_RANGE_NOT_SATISFIABLE;
using net::GetMimeTypeFromFile;
using net::HttpUtil;
using net::OK;
using net::SourceStream;
using net::URLRequestStatus;
using net::GzipSourceStream;

CodeCacheURLRequestJob::FileMetaInfo::FileMetaInfo()
    : file_size(0),
      mime_type_result(false),
      file_exists(false),
      is_directory(false) {}

CodeCacheURLRequestJob::CodeCacheURLRequestJob(
    URLRequest* request,
    NetworkDelegate* network_delegate,
    const base::FilePath& file_path,
    const scoped_refptr<base::TaskRunner>& file_task_runner)
    : net::URLRequestJob(request, network_delegate),
      file_path_(file_path),
      stream_(new FileStream(file_task_runner)),
      file_task_runner_(file_task_runner),
      remaining_bytes_(0),
      waiting_open_entry_(false),
      pending_first_byte_position_(-1),
      range_parse_result_(OK),
      weak_ptr_factory_(this) {}

CodeCacheURLRequestJob::~CodeCacheURLRequestJob() {}

void CodeCacheURLRequestJob::Start() {
  FileMetaInfo* meta_info = new FileMetaInfo();
  file_task_runner_->PostTaskAndReply(
      FROM_HERE, base::Bind(&CodeCacheURLRequestJob::FetchMetaInfo, file_path_,
                            base::Unretained(meta_info)),
      base::Bind(&CodeCacheURLRequestJob::DidFetchMetaInfo,
                 weak_ptr_factory_.GetWeakPtr(), base::Owned(meta_info)));
}

void CodeCacheURLRequestJob::Kill() {
  stream_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();

  URLRequestJob::Kill();
}

int CodeCacheURLRequestJob::ReadRawData(IOBuffer* dest, int dest_size) {
  DCHECK_NE(dest_size, 0);
  DCHECK_GE(remaining_bytes_, 0);

  if (remaining_bytes_ < dest_size)
    dest_size = static_cast<int>(remaining_bytes_);

  // If we should copy zero bytes because |remaining_bytes_| is zero, short
  // circuit here.
  if (!dest_size)
    return 0;

  int rv = stream_->Read(
      dest, dest_size,
      base::Bind(&CodeCacheURLRequestJob::DidRead,
                 weak_ptr_factory_.GetWeakPtr(), base::WrapRefCounted(dest)));
  if (rv >= 0) {
    // Data is immediately available.
    remaining_bytes_ -= rv;
    DCHECK_GE(remaining_bytes_, 0);
  }

  return rv;
}

bool CodeCacheURLRequestJob::IsRedirectResponse(GURL* location,
                                                int* http_status_code) {
  if (meta_info_.is_directory) {
    // This happens when we discovered the file is a directory, so needs a
    // slash at the end of the path.
    std::string new_path = request_->url().path();
    new_path.push_back('/');
    GURL::Replacements replacements;
    replacements.SetPathStr(new_path);

    *location = request_->url().ReplaceComponents(replacements);
    *http_status_code = 301;  // simulate a permanent redirect
    return true;
  }

#if defined(OS_WIN)
  // Follow a Windows shortcut.
  // We just resolve .lnk file, ignore others.
  if (!LowerCaseEqualsASCII(file_path_.Extension(), ".lnk"))
    return false;

  base::FilePath new_path = file_path_;
  bool resolved;
  resolved = base::win::ResolveShortcut(new_path, &new_path, nullptr);

  // If shortcut is not resolved succesfully, do not redirect.
  if (!resolved)
    return false;

  *location = FilePathToFileURL(new_path);
  *http_status_code = 301;
  return true;
#else
  return false;
#endif
}

bool CodeCacheURLRequestJob::GetMimeType(std::string* mime_type) const {
  DCHECK(request_);
  if (meta_info_.mime_type_result) {
    *mime_type = meta_info_.mime_type;
    return true;
  }
  return false;
}

void CodeCacheURLRequestJob::SetExtraRequestHeaders(
    const HttpRequestHeaders& headers) {
  std::string range_header;
  if (headers.GetHeader(HttpRequestHeaders::kRange, &range_header)) {
    // We only care about "Range" header here.
    std::vector<HttpByteRange> ranges;
    if (HttpUtil::ParseRangeHeader(range_header, &ranges)) {
      if (ranges.size() == 1) {
        byte_range_ = ranges[0];
      } else {
        // We don't support multiple range requests in one single URL request,
        // because we need to do multipart encoding here.
        // TODO(hclam): decide whether we want to support multiple range
        // requests.
        NotifyStartError(URLRequestStatus(URLRequestStatus::FAILED,
                                          ERR_REQUEST_RANGE_NOT_SATISFIABLE));
      }
    }
  }
}

void CodeCacheURLRequestJob::OnOpenComplete(int result) {}

void CodeCacheURLRequestJob::OnSeekComplete(int64_t result) {}

void CodeCacheURLRequestJob::OnReadComplete(IOBuffer* buf, int result) {}

std::unique_ptr<SourceStream> CodeCacheURLRequestJob::SetUpSourceStream() {
  std::unique_ptr<SourceStream> source = URLRequestJob::SetUpSourceStream();
  if (!base::LowerCaseEqualsASCII(file_path_.Extension(), ".svgz"))
    return source;

  return GzipSourceStream::Create(std::move(source), SourceStream::TYPE_GZIP);
}

bool CodeCacheURLRequestJob::CanAccessFile(
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) {
  return !network_delegate() ||
         network_delegate()->CanAccessFile(*request(), original_path,
                                           absolute_path);
}

void CodeCacheURLRequestJob::FetchMetaInfo(const base::FilePath& file_path,
                                           FileMetaInfo* meta_info) {
  base::File::Info file_info;
  meta_info->file_exists = base::GetFileInfo(file_path, &file_info);
  if (meta_info->file_exists) {
    meta_info->file_size = file_info.size;
    meta_info->is_directory = file_info.is_directory;
  }
  // On Windows GetMimeTypeFromFile() goes to the registry. Thus it should be
  // done in WorkerPool.
  meta_info->mime_type_result =
      GetMimeTypeFromFile(file_path, &meta_info->mime_type);

  meta_info->absolute_path = base::MakeAbsoluteFilePath(file_path);
  meta_info->last_modified = file_info.last_modified;
}

void CodeCacheURLRequestJob::DidFetchMetaInfo(const FileMetaInfo* meta_info) {
  meta_info_ = *meta_info;
  OpenEntry();

  // We use CodeCacheURLRequestJob to handle files as well as directories
  // without trailing slash.
  // If a directory does not exist, we return ERR_FILE_NOT_FOUND. Otherwise,
  // we will append trailing slash and redirect to FileDirJob.
  // A special case is "\" on Windows. We should resolve as invalid.
  // However, Windows resolves "\" to "C:\", thus reports it as existent.
  // So what happens is we append it with trailing slash and redirect it to
  // FileDirJob where it is resolved as invalid.
  if (!meta_info_.file_exists) {
    DidOpen(ERR_FILE_NOT_FOUND);
    return;
  }
  if (meta_info_.is_directory) {
    DidOpen(OK);
    return;
  }

  if (!CanAccessFile(file_path_, meta_info->absolute_path)) {
    DidOpen(ERR_ACCESS_DENIED);
  }

  int flags =
      base::File::FLAG_OPEN | base::File::FLAG_READ | base::File::FLAG_ASYNC;
  int rv = stream_->Open(file_path_, flags,
                         base::Bind(&CodeCacheURLRequestJob::DidOpen,
                                    weak_ptr_factory_.GetWeakPtr()));
  if (rv != ERR_IO_PENDING)
    DidOpen(rv);
}

void CodeCacheURLRequestJob::DidOpen(int result) {
  OnOpenComplete(result);
  if (result != OK) {
    NotifyStartError(URLRequestStatus(URLRequestStatus::FAILED, result));
    return;
  }

  if (range_parse_result_ != net::OK ||
      !byte_range_.ComputeBounds(meta_info_.file_size)) {
    DidSeek(ERR_REQUEST_RANGE_NOT_SATISFIABLE);
    return;
  }

  remaining_bytes_ =
      byte_range_.last_byte_position() - byte_range_.first_byte_position() + 1;
  DCHECK_GE(remaining_bytes_, 0);

  if (remaining_bytes_ > 0 && byte_range_.first_byte_position() != 0) {
    int rv = stream_->Seek(byte_range_.first_byte_position(),
                           base::Bind(&CodeCacheURLRequestJob::DidSeek,
                                      weak_ptr_factory_.GetWeakPtr()));
    if (rv != ERR_IO_PENDING) {
      DidSeek(ERR_REQUEST_RANGE_NOT_SATISFIABLE);
    }
  } else {
    // We didn't need to call stream_->Seek() at all, so we pass to DidSeek()
    // the value that would mean seek success. This way we skip the code
    // handling seek failure.
    if (waiting_open_entry_)
      pending_first_byte_position_ = byte_range_.first_byte_position();
    else
      DidSeek(byte_range_.first_byte_position());
  }
}

void CodeCacheURLRequestJob::DidSeek(int64_t result) {
  DCHECK(result < 0 || result == byte_range_.first_byte_position());

  OnSeekComplete(result);
  if (result < 0) {
    NotifyStartError(URLRequestStatus(URLRequestStatus::FAILED,
                                      ERR_REQUEST_RANGE_NOT_SATISFIABLE));
    return;
  }

  set_expected_content_size(remaining_bytes_);
  NotifyHeadersComplete();
}

void CodeCacheURLRequestJob::DidRead(scoped_refptr<IOBuffer> buf, int result) {
  if (result > 0) {
    // FIXME(jb) SetState is a private function
    // SetStatus(URLRequestStatus());  // Clear the IO_PENDING status
    remaining_bytes_ -= result;
    DCHECK_GE(remaining_bytes_, 0);
  }

  OnReadComplete(buf.get(), result);
  buf = nullptr;

  if (result < 0)
    NotifyStartError(URLRequestStatus(URLRequestStatus::FAILED, result));

  ReadRawDataComplete(result);
}

void CodeCacheURLRequestJob::OpenEntry() {
  CodeCache* code_cache = request()->context()->code_cache();
  CodeCache::ReadCallback read_callback(base::Bind(
      &CodeCacheURLRequestJob::DidReadData, weak_ptr_factory_.GetWeakPtr()));
  waiting_open_entry_ = true;
  code_cache->ReadMetadata(request()->url(), meta_info_.last_modified,
                           read_callback);
}

void CodeCacheURLRequestJob::DidReadData(int result,
                                         scoped_refptr<IOBufferWithSize> buf) {
  waiting_open_entry_ = false;
  LOG(INFO) << "DidReadData " << (result == OK ? "OK" : "Error");
  if (result == OK) {
    CHECK(buf && buf->size() > 0);
    URLRequest* urlRequest = request();
    if (urlRequest)
      urlRequest->UpdateMetadata(buf);
  }

  if (pending_first_byte_position_ != -1)
    DidSeek(pending_first_byte_position_);
}

}  // namespace content
