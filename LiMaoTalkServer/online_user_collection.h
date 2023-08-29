#pragma once
#include <shared_mutex>
#include <map>
#include <cstdint>
#include "modules_loader.h"
#include "user.h"
#include "task_pool.h"
namespace LiMao::Modules::UserControl
{
	class TextPackWithLogin:public LiMao::Data::DataPackage::TextDataPack
	{
	private:

	public:
		std::string token;
		std::uint64_t uid=0;
		std::string message;
		TextPackWithLogin(void)noexcept;
		TextPackWithLogin(const std::string& text);
		RbsLib::Buffer ToBuffer(void) const override;
		
	};
	class UserControlModule:public LiMao::Modules::IModule
	{
	private:
		LiMao::ID::UUID uuid;
		std::map<std::uint64_t,User> users;
		std::map<std::uint64_t,std::shared_ptr<LiMao::Service::SafeNetwork>> user_connections;
		std::shared_mutex users_mutex,online_connections_mutex;
		LiMao::Service::TaskPool* task_pool;
		bool exit = false;
		bool check_token(std::uint64_t user_id, std::string token)noexcept;
		LiMao::Data::DataPackage::TextDataPack regist_message(LiMao::Data::DataPackage::TextDataPack&pack) const;
		LiMao::Data::DataPackage::TextDataPack login_message(LiMao::Data::DataPackage::TextDataPack& pack,LiMao::Service::SafeNetwork&network);
		LiMao::Data::DataPackage::TextDataPack request_friend(LiMao::Data::DataPackage::TextDataPack& pack);
	public:
		// 通过 IModule 继承
		bool OnLoad(const LiMao::ID::UUID& module_uuid) override;
		bool OnUnload(void) override;
		bool OnMessage(LiMao::Data::DataPackage::DataPack& data, ConnectionInformation& info) override;
		void OnConnectionClosed(const LiMao::Service::SafeNetwork& safe_connection) override;
	};
}
