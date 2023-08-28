#include "uid.h"
#include "configuration_manager.h"
#include "Buffer.h"
#include "Storage.h"
#include "FileIO.h"
#include "configuration_manager.h"
#include <cstring>
#include <inttypes.h>
uint64_t LiMao::ID::UID::Generate(void)
{
	uint64_t uid = 0;
	try
	{
		RbsLib::Storage::StorageFile uid_file(
			LiMao::Config::ConfigManager::UidPoolDir()
		);
		auto lock = LiMao::Config::GlobalFileRWLock::GetMutex(uid_file.Path());
		lock->lock();
		/*检查UID文件夹是否存在*/
		if (!uid_file.IsExist())
		{
			std::filesystem::create_directories(uid_file.Path());
		}
		try
		{
			auto fp = uid_file["next_uid"].Open(RbsLib::Storage::FileIO::OpenMode::Read,
				RbsLib::Storage::FileIO::SeekBase::begin, 0);
			std::string str = fp.GetLine(128);
			uid = std::stoull(str);
		}
		catch (const RbsLib::Storage::FileIO::FileIOException& ex)
		{
			/*可能是文件不存在*/
			auto fp = uid_file["next_uid"].Open(
				RbsLib::Storage::FileIO::OpenMode::Write | RbsLib::Storage::FileIO::OpenMode::Replace,
				RbsLib::Storage::FileIO::SeekBase::begin, 0);
			fp.WriteLine(std::to_string(uid = 10000));
		}
		auto fp = uid_file["next_uid"].Open(
			RbsLib::Storage::FileIO::OpenMode::Write | RbsLib::Storage::FileIO::OpenMode::Replace,
			RbsLib::Storage::FileIO::SeekBase::begin, 0);
		fp.WriteLine(std::to_string(uid + 1));
		lock->unlock();
		return uid;
	}
	catch (const std::exception& e)
	{
		throw IDException(std::string("UID configuration file error:") + e.what());
	}
	return 0;
}
