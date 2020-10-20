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
 * File:   fslogger.h
 */

#ifndef FS_LOGGER_H
#define FS_LOGGER_H

#include "fusepp/Fuse.h"

#include <memory>
#include <string>

#include <stdio.h>

namespace fspf
{

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

class FsPropFaker;

/** Fuse specific logger.
 */
class FsLogger
{
public:
	~FsLogger() noexcept;
	static std::pair<unique_ptr<FsLogger>, std::string> create(const std::string& sMountName, const std::string& sLogFilePath) noexcept;

	//void log_function(const char* p0FuncName, const char* p0Format, ...);
	void log_msg(const char* p0Format, ...);
	void log_conn(struct fuse_conn_info* p0Conn);
	int log_error(const char* p0Func);
	void log_fi(struct fuse_file_info* p0FI);
	void log_fuse_context(struct fuse_context* p0Context);
	void log_retstat(const char* p0Func, int nRetStat);
	void log_stat(struct stat* p0StatBuf);
	void log_statvfs(struct ::statvfs* p0StatFs);
	int  log_syscall(const char* p0Func, int nRetStat, int nMinRet);
	void log_utime(struct utimbuf* p0Buf);
protected:
	FsLogger(const std::string& sMountName, const std::string& sLogFilePath) noexcept;
	std::string init() noexcept;
private:
	const std::string& m_sMountName;
	const std::string& m_sLogFilePath;

	FILE* m_p0LogFile = nullptr;
private:
	FsLogger() = delete;
	FsLogger(const FsLogger& oSource) = delete;
	FsLogger& operator=(const FsLogger& oSource) = delete;
};

} // namespace fspf

#endif /* FS_LOGGER_H */

