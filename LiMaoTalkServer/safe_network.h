#pragma once
#include "Network.h"
#include <exception>
namespace LiMao
{
	namespace Service
	{
		class SafeNetworkException :public std::exception
		{
		private:
			std::string reason;
		public:
			SafeNetworkException(const std::string& reason) noexcept;
			const char* what(void)const noexcept override;
		};
		class SafeNetwork
		{
		private:
			RbsLib::Network::TCP::TCPConnection connection;
			unsigned int max_pack_size;
		public:
			SafeNetwork(const RbsLib::Network::TCP::TCPConnection& connnection,unsigned int max_pack_size);
			void Send(const RbsLib::IBuffer& buffer) const;
			RbsLib::Buffer Recv(void)const;
			auto GetConnection(void)const->RbsLib::Network::TCP::TCPConnection const&;
		};
	}
}