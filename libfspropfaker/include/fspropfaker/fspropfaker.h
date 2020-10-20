/*
 * Copyright © 2020  Stefano Marsili, <stemars@gmx.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>
 */
/*
 * File:   fspropfaker.h
 */

#ifndef FS_PROP_FAKER_H
#define FS_PROP_FAKER_H

#include <utility>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace fspf
{

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

class OverFs;

/** File system property faker.
 */
class FsPropFaker
{
public:
	~FsPropFaker() noexcept;
	struct CreateResult
	{
		unique_ptr<FsPropFaker> m_refFaker; /**< If null an error occurred. */
		std::string m_sError; /**< The error. If empty no error occurred. */
	};
	/** Creates an instance.
	 * If sMountPath is empty '/tmp/fsprofakerNNNNN/' (where N is a random digit) will be created and used.
	 *
	 * Make sure that sLogFilePath is not within sFsFolderPath, that is the mounted fake file system.
	 * @param sMountName The name of the file system. If empty some default is chosen.
	 * @param sFsFolderPath The absolute path of the folder that will be seen as a new fake filesystem.
	 * @param sMountPath The absolute path of the folder that will mount the new fake filesystem. Can be empty.
	 * @param sLogFilePath The file path of the log file. If empty no logging will take place.
	 * @return The result.
	 */
	static CreateResult create(const std::string& sMountName
								, const std::string& sFsFolderPath
								, const std::string& sMountPath
								, const std::string& sLogFilePath) noexcept;

	/** The name of the mounted file system.
	 * @return The name. Is not empty.
	 */
	const std::string& getMountName() const noexcept;
	/** The folder path of the underlying file system.
	 * @return The folder that is mounted to the "fake" file system.
	 */
	const std::string& getRootPath() const noexcept;
	/** The folder path of the mounted file system.
	 * @return The "fake" file system folder.
	 */
	const std::string& getMountPath() const noexcept;
	/** The log file path.
	 * @return The log file path. If empty no logging.
	 */
	const std::string& getLogFilePath() const noexcept;
	/** The block size of the underlying file system.
	 * @return The size of a block in bytes.
	 */
	int64_t getBlockSize() const noexcept;

	//TODO enum operations
	//TODO setOperationDelay
	//TODO setOperationFailure
	//TODO getOperationCount

	/** The number of bytes this class considers a megabyte. */
	static constexpr int64_t s_nMegaByteBytes = 1000000;

	/** The real disk size of the underlying filesystem.
	 * @return The real size in blocks.
	 */
	int64_t getRealDiskSizeInBlocks() noexcept;
	/** The real disk size of the underlying filesystem.
	 * @return The real size in MB.
	 */
	int64_t getRealDiskSizeInMB() noexcept;
	/** The real free disk size of the underlying filesystem.
	 * @return The real free size in blocks.
	 */
	int64_t getRealFreeSizeInBlocks() noexcept;
	/** The real free disk size of the underlying filesystem.
	 * @return The real free size in MB.
	 */
	int64_t getRealFreeSizeInMB() noexcept;

	/** Sets the fake fixed disk size in blocks.
	 * @param nSizeBlocks The new requested fake fixed size in blocks. Must be positive.
	 */
	void setFakeDiskSizeInBlocks(int64_t nSizeBlocks) noexcept;
	/** Sets the fake fixed disk size.
	 * Ex. if the block size is 10 MB and the requested size is 22 MB, then the actual size is 3 blocks (30 MB).
	 *
	 * @param nSizeMB The new requested fake fixed size in megabytes. Must be positive.
	 * @return The actual fake fixed size in blocks.
	 */
	int64_t setFakeDiskSizeInMB(int64_t nSizeMB) noexcept;
	/** Sets the fake disk size as a difference to the real size.
	 * If after applying the difference the fake size is negative it is set to 0.
	 *
	 * @param nSizeBlocks The new requested fake size difference in blocks. If 0 the fake disk size is the same as the real.
	 */
	void setFakeDiskSizeDiffInBlocks(int64_t nSizeBlocks) noexcept;
	/** Sets the fake disk size as a difference to the real size.
	 * If after applying the difference the fake size is negative it is set to 0.
	 *
	 * Ex. if the block size is 10 MB and the requested size difference is -22, then the actual size difference is -30.
	 * So a with a real disk size of 90 the fake size would be `90 - 30 = 60`.
	 *
	 * @param nSizeMB The new requested fake size difference in megabytes. If 0 the fake disk size is the same as the real.
	 * @return The actual fake size difference in blocks.
	 */
	int64_t setFakeDiskSizeDiffInMB(int64_t nSizeMB) noexcept;

	/** Sets the fake fixed free disk size.
	 * If the fake free size is bigger than the fake disk size it is set to the
	 * fake disk size.
	 *
	 * @param nFreeSizeBlocks The new requested fake fixed free size in blocks. Must be positive.
	 */
	void setFakeDiskFreeSizeInBlocks(int64_t nFreeSizeBlocks) noexcept;
	/** Sets the fake fixed free disk size.
	 * If the fake free size is bigger than the fake disk size it is set to the
	 * fake disk size.
	 *
	 * Ex. if the block size is 10 and the requested free size is 22, then the actual free size is 30.
	 * But if fake disk size is 20 the actual value becomes 20.
	 *
	 * @param nFreeSizeMB The new requested fake fixed size in megabytes. Must be positive.
	 * @return The actual fake free size in blocks.
	 */
	int64_t setFakeDiskFreeSizeInMB(int64_t nFreeSizeMB) noexcept;
	/** Sets the fake free disk size as a difference to the real free size.
	 * If after applying the difference the fake free size is bigger than the fake disk size
	 * it is set to the fake disk size.
	 *
	 * @param nFreeSizeBlocks The new requested fake free size difference in blocks. If 0 the fake free disk size is the same as the real.
	 */
	void setFakeDiskFreeSizeDiffInBlocks(int64_t nFreeSizeBlocks) noexcept;
	/** Sets the fake free disk size as a difference to the real free size.
	 * If after applying the difference the fake free size is bigger than the fake disk size
	 * it is set to the fake disk size.
	 *
	 * Ex. if the block size is 10 and the requested size difference is -22, then the actual size difference is -30.
	 * With a real free disk size of 50 MB the fake size would be `50 - 30 = 20`.
	 *
	 * @param nFreeSizeMB The new requested fake free size difference in megabytes. If 0 the fake free disk size is the same as the real.
	 * @return The actual fake free size difference in blocks.
	 */
	int64_t setFakeDiskFreeSizeDiffInMB(int64_t nFreeSizeMB) noexcept;

	/** Unmount the file system.
	 * This should stop the fuse thread.
	 * @return An empty string if successful, an error string otherwise.
	 */
	std::string unmount() noexcept;
protected:
	FsPropFaker() noexcept;
private:
	// return empty if ok, error otherwise
	std::string init(const std::string& sMountName
					, const std::string& sFsFolderPath
					, const std::string& sMountPath
					, const std::string& sLogFilePath) noexcept;
	// return empty if ok, error otherwise
	std::string createThread() noexcept;
	void fuseInitialized() noexcept;
private:
	std::string m_sMountName;
	std::string m_sMountPath;
	std::string m_sRootPath;
	std::string m_sLogFilePath;
	int64_t m_nBlockSize = 1;

	shared_ptr<OverFs> m_refFs;

	unique_ptr<std::thread> m_refFsThread;

	std::mutex m_oFsStartedMutex;
	std::atomic<bool> m_bFileSystemStarted = ATOMIC_VAR_INIT(false);
	std::condition_variable m_oFileSystemStarted;

private:
	FsPropFaker(const FsPropFaker& oSource) = delete;
	FsPropFaker& operator=(const FsPropFaker& oSource) = delete;
};

} // namespace fspf

#endif /* FS_PROP_FAKER_H */

