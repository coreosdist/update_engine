// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_PLATFORM_UPDATE_ENGINE_DELTA_PERFORMER_H__
#define CHROMEOS_PLATFORM_UPDATE_ENGINE_DELTA_PERFORMER_H__

#include <inttypes.h>
#include <vector>
#include <google/protobuf/repeated_field.h>
#include "update_engine/file_writer.h"
#include "update_engine/update_metadata.pb.h"

namespace chromeos_update_engine {

// This class performs the actions in a delta update synchronously. The delta
// update itself should be passed in in chunks as it is received.

class DeltaPerformer : public FileWriter {
 public:
  DeltaPerformer()
      : fd_(-1),
        manifest_valid_(false),
        next_operation_(0),
        buffer_offset_(0),
        block_size_(0) {}
  // flags and mode ignored. Once Close()d, a DeltaPerformer can't be
  // Open()ed again.
  int Open(const char* path, int flags, mode_t mode);

  // Wrapper around write. Returns bytes written on success or
  // -errno on error.
  int Write(const void* bytes, size_t count);

  // Wrapper around close. Returns 0 on success or -errno on error.
  int Close();
  
  // Converts an ordered collection of Extent objects which contain data of
  // length full_length to a comma-separated string. For each Extent, the
  // string will have the start offset and then the length in bytes.
  // The length value of the last extent in the string may be short, since
  // the full length of all extents in the string is capped to full_length.
  // Also, an extent starting at kSparseHole, appears as -1 in the string.
  // For example, if the Extents are {1, 1}, {4, 2}, {kSparseHole, 1},
  // {0, 1}, block_size is 4096, and full_length is 5 * block_size - 13,
  // the resulting string will be: "4096:4096,16384:8192,-1:4096,0:4083"
  static bool ExtentsToBsdiffPositionsString(
      const google::protobuf::RepeatedPtrField<Extent>& extents,
      uint64_t block_size,
      uint64_t full_length,
      std::string* positions_string);

 private:
  // Returns true if enough of the delta file has been passed via Write()
  // to be able to perform a given install operation.
  bool CanPerformInstallOperation(
      const DeltaArchiveManifest_InstallOperation& operation);
  
  // Returns true on success.
  bool PerformInstallOperation(
      const DeltaArchiveManifest_InstallOperation& operation);
  
  // These perform a specific type of operation and return true on success.
  bool PerformReplaceOperation(
      const DeltaArchiveManifest_InstallOperation& operation);
  bool PerformMoveOperation(
      const DeltaArchiveManifest_InstallOperation& operation);
  bool PerformBsdiffOperation(
      const DeltaArchiveManifest_InstallOperation& operation);

  // File descriptor of open device.
  int fd_;
  
  std::string path_;  // Path that fd_ refers to
  
  DeltaArchiveManifest manifest_;
  bool manifest_valid_;
  
  // Index of the next operation to perform in the manifest.
  int next_operation_;

  // buffer_ is a window of the data that's been downloaded. At first,
  // it contains the beginning of the download, but after the protobuf
  // has been downloaded and parsed, it contains a sliding window of
  // data blobs.
  std::vector<char> buffer_;
  // Offset of buffer_ in the binary blobs section of the update.
  uint64_t buffer_offset_;
  
  // The block size (parsed from the manifest).
  uint32_t block_size_;
  
  DISALLOW_COPY_AND_ASSIGN(DeltaPerformer);
};

}  // namespace chromeos_update_engine

#endif  // CHROMEOS_PLATFORM_UPDATE_ENGINE_DELTA_PERFORMER_H__