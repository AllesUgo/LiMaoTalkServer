#pragma once
#include <list>
#include <string>
#include <map>
#include "datapackage.h"
#include "uuid.h"
#include "safe_network.h"
#include "sending_async.h"
//用于管理处理消息处理模块

namespace LiMao
{
	namespace Modules
	{
		class ModuleException :public std::exception
		{
		private:
			std::string reason;
		public:
			ModuleException(std::string const& reason)noexcept;
			const char* what(void)const noexcept override;
		};
		class ConnectionInformation
		{
		public:
			LiMao::Service::SafeNetwork safe_connection;
			LiMao::Service::SendingAsync& sending_service;
			ConnectionInformation(const LiMao::Service::SafeNetwork& safe_connection, LiMao::Service::SendingAsync& sending_service);
		};
		class IModule
		{
		public:
			virtual bool OnLoad(const LiMao::ID::UUID& module_uuid);
			virtual bool OnUnload(void);
			virtual bool OnMessage(LiMao::Data::DataPackage::DataPack& data,ConnectionInformation&info) = 0;//返回值为真表示消息已处理完成，应移除该消息
			virtual void OnConnectionClosed(const LiMao::Service::SafeNetwork& safe_connection);
		};
		class ModulesManager
		{
		private:
			
		public:
			ModulesManager() = delete;
			static LiMao::ID::UUID RegisterModule(IModule* module);
			static bool UnregisterModule(const LiMao::ID::UUID& uuid);
			static void DealMessage(LiMao::Data::DataPackage::DataPack& data,const ConnectionInformation&info);
			static void DealConnectionClosed(const RbsLib::Network::TCP::TCPConnection& client);
			static void UnloadAllModule(void);
		};
	}
}