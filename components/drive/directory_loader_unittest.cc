// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/directory_loader.h"

#include <memory>

#include "base/callback_helpers.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/chromeos/about_resource_loader.h"
#include "components/drive/chromeos/about_resource_root_folder_id_loader.h"
#include "components/drive/chromeos/change_list_loader_observer.h"
#include "components/drive/chromeos/drive_test_util.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/loader_controller.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/chromeos/root_folder_id_loader.h"
#include "components/drive/chromeos/start_page_token_loader.h"
#include "components/drive/event_logger.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/service/fake_drive_service.h"
#include "components/drive/service/test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

namespace {

class TestDirectoryLoaderObserver : public ChangeListLoaderObserver {
 public:
  explicit TestDirectoryLoaderObserver(DirectoryLoader* loader)
      : loader_(loader) {
    loader_->AddObserver(this);
  }

  ~TestDirectoryLoaderObserver() override { loader_->RemoveObserver(this); }

  const std::set<base::FilePath>& changed_directories() const {
    return changed_directories_;
  }
  void clear_changed_directories() { changed_directories_.clear(); }

  // ChageListObserver overrides:
  void OnDirectoryReloaded(const base::FilePath& directory_path) override {
    changed_directories_.insert(directory_path);
  }

 private:
  DirectoryLoader* loader_;
  std::set<base::FilePath> changed_directories_;

  DISALLOW_COPY_AND_ASSIGN(TestDirectoryLoaderObserver);
};

void AccumulateReadDirectoryResult(
    ResourceEntryVector* out_entries,
    std::unique_ptr<ResourceEntryVector> entries) {
  ASSERT_TRUE(entries);
  out_entries->insert(out_entries->end(), entries->begin(), entries->end());
}

class FakeRootFolderIdLoader : public RootFolderIdLoader {
 public:
  explicit FakeRootFolderIdLoader(const std::string& root_folder_id)
      : root_folder_id_(root_folder_id) {}

  void GetRootFolderId(const RootFolderIdCallback& callback) override {
    callback.Run(FILE_ERROR_OK, root_folder_id_);
  }

 private:
  const std::string root_folder_id_;
};

}  // namespace

class DirectoryLoaderTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    pref_service_ = std::make_unique<TestingPrefServiceSimple>();
    test_util::RegisterDrivePrefs(pref_service_->registry());

    logger_ = std::make_unique<EventLogger>();

    drive_service_ = std::make_unique<FakeDriveService>();
    ASSERT_TRUE(test_util::SetUpTestEntries(drive_service_.get()));

    scheduler_ = std::make_unique<JobScheduler>(
        pref_service_.get(), logger_.get(), drive_service_.get(),
        base::ThreadTaskRunnerHandle::Get().get(), nullptr);
    metadata_storage_.reset(new ResourceMetadataStorage(
        temp_dir_.GetPath(), base::ThreadTaskRunnerHandle::Get().get()));
    ASSERT_TRUE(metadata_storage_->Initialize());

    cache_.reset(new FileCache(metadata_storage_.get(), temp_dir_.GetPath(),
                               base::ThreadTaskRunnerHandle::Get().get(),
                               nullptr /* free_disk_space_getter */));
    ASSERT_TRUE(cache_->Initialize());

    metadata_.reset(new ResourceMetadata(
        metadata_storage_.get(), cache_.get(),
        base::ThreadTaskRunnerHandle::Get().get()));
    ASSERT_EQ(FILE_ERROR_OK, metadata_->Initialize());

    about_resource_loader_ =
        std::make_unique<AboutResourceLoader>(scheduler_.get());
    root_folder_id_loader_ = std::make_unique<AboutResourceRootFolderIdLoader>(
        about_resource_loader_.get());
    start_page_token_loader_ = std::make_unique<StartPageTokenLoader>(
        drive::util::kTeamDriveIdDefaultCorpus, scheduler_.get());
    loader_controller_ = std::make_unique<LoaderController>();
    directory_loader_ = std::make_unique<DirectoryLoader>(
        logger_.get(), base::ThreadTaskRunnerHandle::Get().get(),
        metadata_.get(), scheduler_.get(), root_folder_id_loader_.get(),
        start_page_token_loader_.get(), loader_controller_.get(),
        util::GetDriveMyDriveRootPath(),
        drive::util::kTeamDriveIdDefaultCorpus);
  }

  // Adds a new file to the root directory of the service.
  std::unique_ptr<google_apis::FileResource> AddNewFile(
      const std::string& title) {
    google_apis::DriveApiErrorCode error = google_apis::DRIVE_FILE_ERROR;
    std::unique_ptr<google_apis::FileResource> entry;
    drive_service_->AddNewFile(
        "text/plain",
        "content text",
        drive_service_->GetRootResourceId(),
        title,
        false,  // shared_with_me
        google_apis::test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(google_apis::HTTP_CREATED, error);
    return entry;
  }

  // Creates a ResourceEntry for a directory with explicitly set resource_id.
  ResourceEntry CreateDirectoryEntryWithResourceId(
      const std::string& title,
      const std::string& resource_id,
      const std::string& parent_local_id) {
    ResourceEntry entry;
    entry.set_title(title);
    entry.set_resource_id(resource_id);
    entry.set_parent_local_id(parent_local_id);
    entry.mutable_file_info()->set_is_directory(true);
    entry.mutable_directory_specific_info()->set_start_page_token("0");
    return entry;
  }

  void AddTeamDriveRootEntry(const std::string& team_drive_id,
                             const std::string& team_drive_name) {
    std::string local_id;
    ASSERT_EQ(
        FILE_ERROR_OK,
        metadata_->GetIdByPath(util::GetDriveTeamDrivesRootPath(), &local_id));

    std::string root_local_id = local_id;
    ASSERT_EQ(
        FILE_ERROR_OK,
        metadata_->AddEntry(CreateDirectoryEntryWithResourceId(
                                team_drive_name, team_drive_id, root_local_id),
                            &local_id));
  }

  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<EventLogger> logger_;
  std::unique_ptr<FakeDriveService> drive_service_;
  std::unique_ptr<JobScheduler> scheduler_;
  std::unique_ptr<ResourceMetadataStorage, test_util::DestroyHelperForTests>
      metadata_storage_;
  std::unique_ptr<FileCache, test_util::DestroyHelperForTests> cache_;
  std::unique_ptr<ResourceMetadata, test_util::DestroyHelperForTests> metadata_;
  std::unique_ptr<AboutResourceLoader> about_resource_loader_;
  std::unique_ptr<StartPageTokenLoader> start_page_token_loader_;
  std::unique_ptr<LoaderController> loader_controller_;
  std::unique_ptr<DirectoryLoader> directory_loader_;
  std::unique_ptr<AboutResourceRootFolderIdLoader> root_folder_id_loader_;
};

TEST_F(DirectoryLoaderTest, ReadDirectory_GrandRoot) {
  TestDirectoryLoaderObserver observer(directory_loader_.get());

  // Load grand root.
  FileError error = FILE_ERROR_FAILED;
  ResourceEntryVector entries;
  directory_loader_->ReadDirectory(
      util::GetDriveGrandRootPath(),
      base::Bind(&AccumulateReadDirectoryResult, &entries),
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(0U, observer.changed_directories().size());
  observer.clear_changed_directories();

  // My Drive has resource ID.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryByPath(util::GetDriveMyDriveRootPath(),
                                              &entry));
  EXPECT_EQ(drive_service_->GetRootResourceId(), entry.resource_id());
}

TEST_F(DirectoryLoaderTest, ReadDirectory_MyDrive) {
  TestDirectoryLoaderObserver observer(directory_loader_.get());

  // My Drive does not have resource ID yet.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryByPath(util::GetDriveMyDriveRootPath(),
                                              &entry));
  EXPECT_TRUE(entry.resource_id().empty());

  // Load My Drive.
  FileError error = FILE_ERROR_FAILED;
  ResourceEntryVector entries;
  directory_loader_->ReadDirectory(
      util::GetDriveMyDriveRootPath(),
      base::Bind(&AccumulateReadDirectoryResult, &entries),
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(1U, observer.changed_directories().count(
      util::GetDriveMyDriveRootPath()));

  // My Drive has resource ID.
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryByPath(util::GetDriveMyDriveRootPath(),
                                              &entry));
  EXPECT_EQ(drive_service_->GetRootResourceId(), entry.resource_id());
  EXPECT_EQ(drive_service_->start_page_token().start_page_token(),
            entry.directory_specific_info().start_page_token());

  // My Drive's child is present.
  base::FilePath file_path =
      util::GetDriveMyDriveRootPath().AppendASCII("File 1.txt");
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryByPath(file_path, &entry));
}

TEST_F(DirectoryLoaderTest, ReadDirectory_MultipleCalls) {
  TestDirectoryLoaderObserver observer(directory_loader_.get());

  // Load grand root.
  FileError error = FILE_ERROR_FAILED;
  ResourceEntryVector entries;
  directory_loader_->ReadDirectory(
      util::GetDriveGrandRootPath(),
      base::Bind(&AccumulateReadDirectoryResult, &entries),
      google_apis::test_util::CreateCopyResultCallback(&error));

  // Load grand root again without waiting for the result.
  FileError error2 = FILE_ERROR_FAILED;
  ResourceEntryVector entries2;
  directory_loader_->ReadDirectory(
      util::GetDriveGrandRootPath(),
      base::Bind(&AccumulateReadDirectoryResult, &entries2),
      google_apis::test_util::CreateCopyResultCallback(&error2));
  base::RunLoop().RunUntilIdle();

  // Callback is called for each method call.
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(FILE_ERROR_OK, error2);
}

TEST_F(DirectoryLoaderTest, Lock) {
  // Lock the loader.
  std::unique_ptr<base::ScopedClosureRunner> lock =
      loader_controller_->GetLock();

  // Start loading.
  TestDirectoryLoaderObserver observer(directory_loader_.get());
  FileError error = FILE_ERROR_FAILED;
  ResourceEntryVector entries;
  directory_loader_->ReadDirectory(
      util::GetDriveMyDriveRootPath(),
      base::Bind(&AccumulateReadDirectoryResult, &entries),
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  // Update is pending due to the lock.
  EXPECT_TRUE(observer.changed_directories().empty());

  // Unlock the loader, this should resume the pending udpate.
  lock.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1U, observer.changed_directories().count(
      util::GetDriveMyDriveRootPath()));
}

TEST_F(DirectoryLoaderTest, TeamDrive) {
  constexpr char kTeamDriveId[] = "team_drive_id";
  constexpr char kTeamDriveName[] = "Team Drive";
  constexpr char kTeamDriveStartPageToken[] = "12345";
  const base::FilePath team_drive_path =
      util::GetDriveTeamDrivesRootPath().AppendASCII(kTeamDriveName);

  auto fake_root_folder_id_loader =
      std::make_unique<FakeRootFolderIdLoader>(kTeamDriveId);
  auto start_page_token_loader =
      std::make_unique<StartPageTokenLoader>(kTeamDriveId, scheduler_.get());
  auto local_directory_loader = std::make_unique<DirectoryLoader>(
      logger_.get(), base::ThreadTaskRunnerHandle::Get().get(), metadata_.get(),
      scheduler_.get(), fake_root_folder_id_loader.get(),
      start_page_token_loader.get(), loader_controller_.get(), team_drive_path,
      kTeamDriveId);

  AddTeamDriveRootEntry(kTeamDriveId, kTeamDriveName);

  drive_service_->AddTeamDrive(kTeamDriveId, kTeamDriveName,
                               kTeamDriveStartPageToken);

  FileError error = FILE_ERROR_FAILED;
  ResourceEntryVector entries;

  local_directory_loader->ReadDirectory(
      team_drive_path, base::Bind(&AccumulateReadDirectoryResult, &entries),
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_EQ(1, drive_service_->start_page_token_load_count());
  EXPECT_EQ(1, drive_service_->directory_load_count());

  // If we checked the folder, we should see that it's start page token has been
  // updated.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryByPath(team_drive_path, &entry));
  EXPECT_EQ(kTeamDriveStartPageToken,
            entry.directory_specific_info().start_page_token());
}
}  // namespace internal
}  // namespace drive
