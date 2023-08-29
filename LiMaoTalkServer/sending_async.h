#pragma once
#include <map>
#include <functional>
#include <mutex>
#include "safe_network.h"
#include "task_pool.h"
#include "message_queue.h"

namespace LiMao
{
	namespace Service
	{
		class SendingAsync
		{
		private:
			class SendingPack
			{
			private:
				RbsLib::Buffer buffer;
				LiMao::Service::SafeNetwork safe_network;
				std::function<void(bool)> callback=nullptr;
			public:
				SendingPack(const RbsLib::Buffer& buffer, const LiMao::Service::SafeNetwork& safe_network, std::function<void(bool)> callback);
				void Send(void);
			};
			std::map<std::string, LiMao::Service::MessageQueue<SendingPack>*> safe_networks;
			LiMao::Service::TaskPool pool;
			std::mutex mutex;//防止多线程发送时访问map冲突
		public:
			SendingAsync(int keep_thread = 0);
			//从队列中发送
			void Send(const std::string &queue_name,const LiMao::Service::SafeNetwork& network, const RbsLib::Buffer& buffer,std::function<void(bool)> send_finished_function=nullptr);
			//无队列发送
			void Send(const LiMao::Service::SafeNetwork& network, const RbsLib::Buffer& buffer, std::function<void(bool)> send_finished_function = nullptr);
		};
	}
}