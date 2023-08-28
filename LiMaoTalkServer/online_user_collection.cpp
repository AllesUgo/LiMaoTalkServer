#include "online_user_collection.h"
#include "logger.h"
#include "uid.h"
#include <shared_mutex>

bool LiMao::Modules::UserControl::UserControlModule::check_token(std::uint64_t user_id, std::string token) noexcept
{
	using namespace std;
	namespace us = LiMao::Modules::UserControl;
	shared_lock<shared_mutex> lock(this->users_mutex);
	auto it = this->users.find(user_id);
	if (it == this->users.end()) return false;
	if (it->second.token != token) return false;
	return us::User::IsTokenAllow(token);
}

LiMao::Data::DataPackage::TextDataPack LiMao::Modules::UserControl::UserControlModule::regist_message(LiMao::Data::DataPackage::TextDataPack& pack) const
{
	std::string username = pack.row_json_obj["data"]("UserName");
	std::string password = pack.row_json_obj["data"]("PassWord");
	if (username.empty() || username.length() > 40)
	{
		neb::CJsonObject json;
		json.Add("ID", 101);
		json.Add("State", 1);
		json.Add("Message", "�û������Ϸ�,�û������ȱ�����1-20�ֽ�֮��");
		return LiMao::Data::DataPackage::TextDataPack(json.ToString());
	}
	if (password.length() < 6 || password.length() > 20)
	{
		neb::CJsonObject json;
		json.Add("ID", 101);
		json.Add("State", 2);
		json.Add("Message", "���벻�Ϸ�,���볤�ȱ�����6-20�ֽ�֮��");
		return LiMao::Data::DataPackage::TextDataPack(json.ToString());
	}
	try
	{
		auto user = LiMao::Modules::UserControl::User::Create(LiMao::ID::UID::Generate(),
			username, password);
		neb::CJsonObject json, subjson;
		json.Add("ID", 101);
		json.Add("State", 0);
		json.Add("Message", "Success");
		json.AddEmptySubObject("Data");
		json["Data"].Add("UID", user.uid);

		return LiMao::Data::DataPackage::TextDataPack(json.ToString());
	}
	catch (UserControlException const& ex)
	{
		if (ex.is_normal_exception)
		{
			neb::CJsonObject json;
			json.Add("ID", 101);
			json.Add("State", -1);
			json.Add("Message", ex.what());
			return LiMao::Data::DataPackage::TextDataPack(json.ToString()).ToBuffer();
		}
		else throw;
	}
}

LiMao::Data::DataPackage::TextDataPack LiMao::Modules::UserControl::UserControlModule::login_message(LiMao::Data::DataPackage::TextDataPack& pack, LiMao::Service::SafeNetwork& network)
{
	namespace us = LiMao::Modules::UserControl;
	uint64_t uid_int = 0;
	std::stringstream uid(pack.row_json_obj["Data"]("UID"));
	std::string password = pack.row_json_obj["Data"]("PassWord");
	uid >> uid_int;
	try
	{
		us::User user = us::User::Login(uid_int, password);
		std::unique_lock<std::shared_mutex> lock(this->users_mutex);
		this->users[uid_int] = user;
		lock.unlock();
		std::unique_lock<std::shared_mutex> lock2(this->online_connections_mutex);
		this->user_connections[uid_int] = std::make_shared<LiMao::Service::SafeNetwork>(network);
		lock2.unlock();
		neb::CJsonObject json, subjson;
		json.Add("ID", 100);
		json.Add("State", 0);
		json.Add("Message", "Success");
		subjson.Add("Token", user.token);
		json.Add("Data", subjson);
		return LiMao::Data::DataPackage::TextDataPack(json.ToFormattedString());
	}
	catch (const us::UserControlException& ex)
	{
		if (ex.is_normal_exception)
		{
			neb::CJsonObject json;
			json.Add("ID", 100);
			json.Add("State", -1);
			json.Add("Message", ex.what());
			return LiMao::Data::DataPackage::TextDataPack(json.ToFormattedString());
		}
		else throw;
	}

}

LiMao::Data::DataPackage::TextDataPack LiMao::Modules::UserControl::UserControlModule::request_friend(LiMao::Data::DataPackage::TextDataPack& pack)
{
	TextPackWithLogin text_pack(pack.ToBuffer().ToString());
	if (!this->check_token(text_pack.uid, text_pack.token))
	{
		TextPackWithLogin pack;
		pack.id= 102;
		pack.message = "Invalid token";
		pack.token = text_pack.token;
		pack.state = 1;
	}
}

bool LiMao::Modules::UserControl::UserControlModule::OnLoad(const LiMao::ID::UUID& module_uuid)
{
	this->task_pool = std::make_shared<LiMao::Service::TaskPool>();
	this->task_pool->Run([this]() {
		std::shared_lock<std::shared_mutex> lock(this->users_mutex);
		lock.unlock();
		while (!this->exit)
		{
			std::this_thread::sleep_for(std::chrono::seconds(10));
			//�ռ������û���token���ڵ��û�
			std::vector<std::uint64_t> expired_users;
			lock.lock();
			for (auto& it : this->users)
			{
				if (!it.second.IsTokenAllow())
				{
					expired_users.push_back(it.first);
				}
			}
			lock.unlock();
			if (!expired_users.empty())
			{
				//ɾ�������û�
				std::unique_lock<std::shared_mutex> lock(this->users_mutex);
				std::unique_lock<std::shared_mutex> lock2(this->online_connections_mutex);
				for (auto& it : expired_users)
				{
					this->users.erase(it);
					this->user_connections.erase(it);
				}
				lock.unlock();
				lock2.unlock();
			}
		}
		LiMao::Service::Logger::LogInfo("Auto clean thread exit");
		});
	this->uuid = module_uuid;
	LiMao::Service::Logger::LogInfo("User module loaded,uuid = %s", module_uuid.ToString().c_str());
	return true;
}

bool LiMao::Modules::UserControl::UserControlModule::OnUnload(void)
{
	this->exit = true;
	LiMao::Service::Logger::LogInfo("User module unloaded,uuid = %s", this->uuid.ToString().c_str());
	return true;
}

bool LiMao::Modules::UserControl::UserControlModule::OnMessage(LiMao::Data::DataPackage::DataPack& data, ConnectionInformation& info)
{
	try
	{
		switch (data.ID())
		{
		case 100://login
			info.sending_service.Send(info.safe_connection, this->login_message(dynamic_cast<LiMao::Data::DataPackage::TextDataPack&>(data), info.safe_connection).ToBuffer());
			break;
		case 101://regist
			info.sending_service.Send(info.safe_connection, this->regist_message(dynamic_cast<LiMao::Data::DataPackage::TextDataPack&>(data)).ToBuffer());
			break;
		default:
			return false;
		}
		return true;
	}
	catch (std::exception const& ex)
	{
		LiMao::Service::Logger::LogWarn("User module exception:%s", ex.what());
		return false;
	}
}

void LiMao::Modules::UserControl::UserControlModule::OnConnectionClosed(const LiMao::Service::SafeNetwork& safe_connection)
{
	std::unique_lock<std::shared_mutex> lock(this->online_connections_mutex);
	for (auto& it : this->user_connections)
	{
		if (it.second->GetConnection() == safe_connection.GetConnection())
		{
			this->user_connections.erase(it.first);
			return;
		}
	}
}

LiMao::Modules::UserControl::TextPackWithLogin::TextPackWithLogin(void) noexcept {}

LiMao::Modules::UserControl::TextPackWithLogin::TextPackWithLogin(const std::string& text)
	:TextDataPack(text)
{
	this->token = this->row_json_obj("Token");
	this->uid = 0;
	std::stringstream(this->row_json_obj("UID")) >> this->uid;
	this->message = this->row_json_obj("Message");
}

RbsLib::Buffer LiMao::Modules::UserControl::TextPackWithLogin::ToBuffer(void) const
{
	neb::CJsonObject json = this->row_json_obj;
	json.ReplaceAdd("ID", this->id);
	json.ReplaceAdd("State", this->state);
	if (!this->message.empty())
		json.ReplaceAdd("Message", this->message);
	else json.ReplaceAdd("Message", "Success");
	json.ReplaceAdd("Token", this->token);
	json.ReplaceAdd("UID", this->uid);
	return LiMao::Data::DataPackage::TextDataPack(json.ToFormattedString()).ToBuffer();
}
