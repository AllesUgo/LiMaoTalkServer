#pragma once
#include "Network.h"
#include <list>
#include <mutex>
namespace LiMao
{
	namespace ConnectionControl
	{
		class ConnectionControlException :public std::exception
		{
		private:
			std::string result;
		public:
			ConnectionControlException(const std::string&reason)noexcept:result(reason){}
			const char* what(void)const noexcept override { return this->result.c_str(); }
		};
		class ConnectionPool
		{
		private:
			std::list<RbsLib::Network::TCP::TCPConnection> connections;
			std::list<RbsLib::Network::TCP::TCPServer> servers;
			int loop_fd[2];
			std::mutex mutex;
		public:
			ConnectionPool();
			~ConnectionPool()noexcept;
			void Add(const RbsLib::Network::TCP::TCPConnection& connection);
			void Add(const RbsLib::Network::TCP::TCPServer& server);
			std::list<RbsLib::Network::TCP::TCPConnection> WaitReadableActive(int32_t timeout);
			void Remove(const RbsLib::Network::TCP::TCPConnection& connection);
			void Remove(SOCKET sock);
			void AlarmWaitReadableActive(void);
		};
	}
}