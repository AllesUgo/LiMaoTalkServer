#include "sending_async.h"
#include "logger.h"
#include <thread>

LiMao::Service::SendingAsync::SendingPack::SendingPack(const RbsLib::Buffer& buffer, const LiMao::Service::SafeNetwork& safe_network, std::function<void(bool)> callback)
	:buffer(buffer), safe_network(safe_network),callback(callback) {}

void LiMao::Service::SendingAsync::SendingPack::Send(void)
{
	try
	{
		this->safe_network.Send(this->buffer);
		if (this->callback!=nullptr) this->callback(true);
	}
	catch (...)
	{
		if (this->callback!=nullptr) this->callback(false);
	}
}

LiMao::Service::SendingAsync::SendingAsync(int keep_thread)
	:pool(keep_thread) {}

void LiMao::Service::SendingAsync::Send(const std::string& queue_name, const LiMao::Service::SafeNetwork& network, const RbsLib::Buffer& buffer, std::function<void(bool)> send_finished_function)
{
	std::unique_lock lock(this->mutex);
	if (this->safe_networks.find(queue_name) == this->safe_networks.end())
	{
		/*当前没有这个队列，为其创建队列*/
		this->safe_networks[queue_name] = new LiMao::Service::MessageQueue<SendingPack>(-1);
		this->safe_networks[queue_name]->AddItem(SendingPack(buffer, network, send_finished_function));
		/*为该队列创建处理线程*/
		auto& mutex = this->mutex;
		auto & safe_network = this->safe_networks;
		std::thread([&mutex,&safe_network,queue_name]() {
			/*取出线程所属队列*/
			std::unique_lock lock(mutex);
			if (safe_network.find(queue_name) == safe_network.end())
			{
				/*队列已经被删除或不存在*/
				return;
			}
			LiMao::Service::MessageQueue<SendingPack>* queue = safe_network[queue_name];
			lock.unlock();
			/*循环发送*/
			while (1)
			{
				try
				{
					lock.lock();
					auto sending = queue->GetItem(0);
					lock.unlock();
					sending.Send();
				}
				catch (const LiMao::Service::MessageQueueException& ex)
				{
					if (ex.type == LiMao::Service::MessageQueueException::Type::TimeOut)
					{
						/*该列表中已无数据，移除该列表*/
						safe_network.erase(queue_name);
						delete queue;
						break;
					}
					else
					{
						/*队列异常，重新抛出*/
						throw;
					}
				}
			}
			}).detach();
	}
	else
	{
		/*已经存在该队列，加入到队列*/
		this->safe_networks[queue_name]->AddItem(SendingPack(buffer, network, send_finished_function));
	}
}

void LiMao::Service::SendingAsync::Send(const LiMao::Service::SafeNetwork& network, const RbsLib::Buffer& buffer, std::function<void(bool)> send_finished_function)
{
	/*无队列发送*/
	try
	{
		SendingPack(buffer, network, send_finished_function).Send();
	}
	catch (std::exception const& ex)
	{
		LiMao::Service::Logger::LogError("%s", ex.what());
	}
}
