#pragma once
#include <exception>
#include <string>
#include <mutex>
#include "BaseType.h"
#include "Buffer.h"
#ifdef WIN32
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
typedef int socklen_t;
#endif // win32

#ifdef LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR SO_ERROR
typedef int SOCKET;
#endif

namespace RbsLib
{
	namespace Network
	{
#ifdef WIN32
		static bool network_inited = 0;
#endif // WIN32
		void init_network();

		class NetworkException:public std::exception
		{
		private:
			std::string reason;
		public:
			NetworkException(const std::string&str)noexcept:reason(str){}
			const char* what(void)const noexcept override { return reason.c_str(); }
		};
		namespace TCP
		{
			class TCPServer;
			class TCPConnection;
			class TCPClient;
			class TCPConnection
			{
				/*同一时间同一连接对象只能有一个线程进行拷贝、移动、析构*/
			private:
				SOCKET sock;
				struct sockaddr_in connection_info;
				int info_len;
				std::mutex*mutex=nullptr;/*多线程保护引用计数器*/
				int* reference_counter=nullptr;
				TCPConnection(SOCKET sock,const struct sockaddr_in& connection_info,int info_len)noexcept;
				friend class TCPServer;
				friend class TCPClient;
			public:
				TCPConnection(const TCPConnection& connection) noexcept;
				TCPConnection(TCPConnection&& connection) noexcept;
				~TCPConnection(void)noexcept;
				const RbsLib::Network::TCP::TCPConnection& operator=(const TCPConnection& conection) noexcept;
				const RbsLib::Network::TCP::TCPConnection& operator=(TCPConnection&& connection) noexcept;
				bool operator==(const TCPConnection& connection)const noexcept;
				void Send(const RbsLib::IBuffer& buffer,int flag=0) const;
				int Send(const void* data, int len, int flag=0) const noexcept;
				RbsLib::Buffer Recv(int len,int flag=0) const;
				int Recv(RbsLib::Buffer& buffer, int flag = 0) const;
				int Recv(void* data, int len, int flag = 0)const noexcept;
				SOCKET GetSocket(void)const noexcept;
				std::string GetAddress(void)const noexcept;
				void Close(void);
			};
			class TCPServer
			{
			private:
				SOCKET server_socket=0;
				bool is_listen=false;
				bool is_bind = false;
				int* reference_counter=nullptr;
			public:
				TCPServer();
				TCPServer(int port, const std::string& address="0.0.0.0");
				TCPServer(const TCPServer& server) noexcept;
				~TCPServer(void)noexcept;
				const TCPServer& operator=(const TCPServer& server)noexcept;
				SOCKET GetSocket(void)const noexcept;
				void Bind(int port, const std::string& address = "0.0.0.0");
				void Listen(int listen_num = 5);
				RbsLib::Network::TCP::TCPConnection Accept(void);
				void Close(void) noexcept;
			};
			class TCPClient
			{
			public:
				static RbsLib::Network::TCP::TCPConnection Connect(std::string ip,int port);
			};
		}
	}
}