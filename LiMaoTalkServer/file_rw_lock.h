#pragma once
#include <mutex>
#include <shared_mutex>
#include <map>
#include <memory>
#include <atomic>
#include <list>
#include <sys/stat.h>
namespace LiMao::Service::FileRWLock
{
	class FileRWLock
	{
	private:
		class MutexGuard
		{
		public:
			std::shared_mutex* mutex;
			std::string file_path;
			unsigned int ref_count = 0;
			MutexGuard(const std::string&path);
		};
		std::mutex mutex;
		std::list<MutexGuard> rw_locks;
	public:
		~FileRWLock()noexcept;
		auto GetMutex(const std::string &file_path) -> std::shared_ptr<std::shared_mutex>;

	};
}