#include "file_rw_lock.h"
#include <thread>

LiMao::Service::FileRWLock::FileRWLock::MutexGuard::MutexGuard(const std::string&path)
	: mutex(new std::shared_mutex),file_path(path){}

LiMao::Service::FileRWLock::FileRWLock::~FileRWLock() noexcept
{
	while (1)
	{
		this->mutex.lock();
		if (this->rw_locks.empty())
		{
			this->mutex.unlock();
			break;
		}
		else
		{
			this->mutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

auto LiMao::Service::FileRWLock::FileRWLock::GetMutex(const std::string &file_path) -> std::shared_ptr<std::shared_mutex>
{
	/*操作列表加锁*/
	this->mutex.lock();
	for (auto& it : this->rw_locks)
	{
		if (it.file_path==file_path)
		{
			/*是同一个文件,增加引用计数并返回锁的共享指针,在共享指针释放时引用计数-1*/
			it.ref_count++;
			auto& rw_locks = this->rw_locks;
			auto& safe_lock = this->mutex;
			std::shared_ptr<std::shared_mutex> ret(it.mutex, [&rw_locks, &safe_lock, &it](std::shared_mutex* mutex)
				{
					safe_lock.lock();
					--it.ref_count;
					if (it.ref_count == 0)
					{
						rw_locks.remove_if([mutex](const MutexGuard& guard)
							{
								return guard.mutex==mutex;
							});
						delete mutex;
					}
					safe_lock.unlock();
				});
			this->mutex.unlock();
			return ret;
		}
	}
	/*列表中没有相同项，加入列表*/
	rw_locks.push_back(MutexGuard(file_path));
	for (auto& it : this->rw_locks)
	{
		if (file_path==it.file_path)
		{
			/*是同一个文件,增加引用计数并返回锁的共享指针,在共享指针释放时引用计数-1*/
			it.ref_count++;
			auto& rw_locks = this->rw_locks;
			auto& safe_lock = this->mutex;
			std::shared_ptr<std::shared_mutex> ret(it.mutex, [&rw_locks, &safe_lock, &it](std::shared_mutex* mutex)
				{
					safe_lock.lock();
					--it.ref_count;
					if (it.ref_count == 0)
					{
						delete mutex;
						rw_locks.remove_if([mutex](const MutexGuard& guard)
							{
								return guard.mutex == mutex;
							});
					}
					safe_lock.unlock();
				});
			this->mutex.unlock();
			return ret;
		}
	}
	abort();
}
