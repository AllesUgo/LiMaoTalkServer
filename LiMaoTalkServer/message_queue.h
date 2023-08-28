#pragma once
#include <cstdint>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <chrono>
namespace LiMao
{
	namespace Service
	{
		class MessageQueueException :public std::exception
		{
		public:
			enum class Type
			{
				None,
				TimeOut
			} type;
			MessageQueueException(Type type, const std::string& reason)noexcept;
			const char* what(void)const noexcept override;
		private:
			std::string reason;

		};
		template <typename ItemType>
		class MessageQueue
		{
		private:
			int64_t max_msg_num;
			std::queue<ItemType> que;
			std::mutex mtx;
			std::condition_variable getter, adder;
		public:
			MessageQueue(int64_t max_mesage_num=0) noexcept;/*0 means unlimited*/
			ItemType GetItem(int time_out = -1);/*-1 means wait inf*/
			ItemType PeekItem(int time_out = -1);
			void AddItem(const ItemType& item,int time_out=-1);/*-1 means wait inf*/
			bool IsIn(const ItemType& item) const noexcept;
			void Clear(void);
		};

		template<typename ItemType>
		inline MessageQueue<ItemType>::MessageQueue(int64_t max_mesage_num) noexcept
		{
			this->max_msg_num = max_mesage_num;
		}
		template<typename ItemType>
		inline ItemType MessageQueue<ItemType>::GetItem(int time_out)
		{
			std::unique_lock<std::mutex> lck(mtx);
			if (!this->que.empty())
			{
				ItemType item = this->que.front();
				this->que.pop();
				lck.unlock();
				this->adder.notify_one();
				return item;
			}
			if (time_out == 0)
			{
				lck.unlock();
				throw MessageQueueException(MessageQueueException::Type::TimeOut, "Get message time out");
			}
			else if (time_out < 0)
			{
				/*Infinite waiting*/
				auto& que = this->que;
				this->getter.wait(lck, [&que]() {return !que.empty(); });
			}
			else
			{
				auto& que = this->que;
				if (this->getter.wait_for(lck, std::chrono::seconds(time_out), [&que]() {return !que.empty(); })
					== false)
				{
					lck.unlock();
					throw MessageQueueException(MessageQueueException::Type::TimeOut, "Get message time out");
				}
			}
			ItemType item = this->que.front();
			this->que.pop();
			lck.unlock();
			this->adder.notify_one();
			return item;
		}
		template<typename ItemType>
		inline ItemType MessageQueue<ItemType>::PeekItem(int time_out)
		{
			std::unique_lock<std::mutex> lck(mtx);
			if (!this->que.empty())
			{
				ItemType item = this->que.front();
				lck.unlock();
				this->adder.notify_one();
				return item;
			}
			if (time_out == 0)
			{
				lck.unlock();
				throw MessageQueueException(MessageQueueException::Type::TimeOut, "Get message time out");
			}
			else if (time_out < 0)
			{
				/*Infinite waiting*/
				auto& que = this->que;
				this->getter.wait(lck, [&que]() {return !que.empty(); });
			}
			else
			{
				auto& que = this->que;
				if (this->getter.wait_for(lck, std::chrono::seconds(time_out), [&que]() {return !que.empty(); })
					== false)
				{
					lck.unlock();
					throw MessageQueueException(MessageQueueException::Type::TimeOut, "Peek message time out");
				}
			}
			ItemType item = this->que.front();
			lck.unlock();
			this->adder.notify_one();
			return item;
		}
		template<typename ItemType>
		inline void MessageQueue<ItemType>::AddItem(const ItemType& item, int time_out)
		{
			std::unique_lock<std::mutex> lck(mtx);
			if (this->que.size() < this->max_msg_num)
			{
				this->que.push(item);
				lck.unlock();
			}
			else
			{
				if (!time_out)
				{
					lck.unlock();
					throw MessageQueueException(MessageQueueException::Type::TimeOut, "Add message time out");
				}
				else if (time_out < 0)
				{
					/*Infinite waiting*/
					auto& que = this->que;
					int64_t& max = this->max_msg_num;
					this->adder.wait(lck, [&que, &max]() {return que.size() < max; });
				}
				else
				{
					/*waitting for a time*/
					auto& que = this->que;
					int64_t& max = this->max_msg_num;
					if (this->adder.wait_for(lck, std::chrono::seconds(time_out), [&que, &max]() {return que.size() < max; })
						== false)
					{
						lck.unlock();
						throw MessageQueueException(MessageQueueException::Type::TimeOut, "Add message time out");
					}
				}
				this->que.push(item);
				lck.unlock();			
			}
			this->getter.notify_one();
		}
		template<typename ItemType>
		inline bool MessageQueue<ItemType>::IsIn(const ItemType& item) const noexcept
		{
			std::unique_lock<std::mutex> lck(mtx);
			for (const auto& i : this->que)
			{
				if (i == item)
				{
					return true;
				}
			}
			return false;
		}
		template<typename ItemType>
		inline void MessageQueue<ItemType>::Clear(void)
		{
			std::unique_lock<std::mutex> lck(mtx);
			this->que = std::queue<ItemType>();
		}
	}
}