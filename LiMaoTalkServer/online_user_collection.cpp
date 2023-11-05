#include "online_user_collection.h"
#include "logger.h"
#include "uid.h"
#include <shared_mutex>
#include "sqlite_cpp.h"
#include "DataBase.h"
#include <fmt/format.h>
#include "sessions.h"

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
		json.Add("Message", "用户名不合法,用户名长度必须在1-20字节之间");
		return LiMao::Data::DataPackage::TextDataPack(json.ToString());
	}
	if (password.length() < 6 || password.length() > 20)
	{
		neb::CJsonObject json;
		json.Add("ID", 101);
		json.Add("State", 2);
		json.Add("Message", "密码不合法,密码长度必须在6-20字节之间");
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
		json["Data"].Add("UID", std::to_string(user.uid));

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
	namespace us = LiMao::Modules::UserControl;
	TextPackWithLogin text_pack(pack.ToBuffer().ToString());
	if (!this->check_token(text_pack.uid, text_pack.token))
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 1, "", "Invalide token").ToBuffer();
	}
	if (!text_pack.row_json_obj["Data"].KeyExist("FriendUID"))
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 2, text_pack.token, "Key:FriendUID not found").ToBuffer();
	}
	uint64_t friend_uid = 0;
	std::stringstream(text_pack.row_json_obj["Data"]("FriendUID")) >> friend_uid;
	if (friend_uid == 0)
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, "Invalide friend uid").ToBuffer();
	}
	if (friend_uid == text_pack.uid)
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 4, text_pack.token, "Can not add self as friend").ToBuffer();
	}
	try
	{
		/*获取当前用户的好友列表*/
		us::User user;
		user.uid = text_pack.uid;
		user.token = text_pack.token;
		auto friends = user.GetFriendList();
		for (auto it : friends)
		{
			if (it == friend_uid)
			{
				return TextPackWithLogin(text_pack.ID(), text_pack.uid, 3, text_pack.token, "Already friend").ToBuffer();
			}
		}
		//尝试通知好友
		std::shared_lock<std::shared_mutex> lock1(this->online_connections_mutex);
		std::shared_lock<std::shared_mutex> lock2(this->users_mutex);
		if (this->user_connections.find(friend_uid) != this->user_connections.end() &&
			this->users.find(friend_uid) != this->users.end())
		{
			neb::CJsonObject json;
			json.Add("ID", 102);
			json.Add("Data", text_pack.row_json_obj["Data"]);
			json.Add("State", 100);
			json.Add("Token", this->users[friend_uid].token);
			json.Add("UID", friend_uid);
			json.AddEmptySubObject("Data");
			json["Data"].Add("RequestUID", text_pack.uid);
			json["Data"].Add("Message", text_pack.row_json_obj["Data"]("Message"));
			this->user_connections[friend_uid]->Send(LiMao::Data::DataPackage::TextDataPack(json.ToFormattedString()).ToBuffer());
		}
		//添加好友请求
		user.uid = text_pack.uid;
		user.SendFriendRequest(friend_uid, text_pack.row_json_obj["data"]("Message"));
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 0, text_pack.token, "Success").ToBuffer();
	}
	catch (const UserControlException& ex)
	{
		if (ex.is_normal_exception)
		{
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, ex.what()).ToBuffer();
		}
		else
		{
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, "Server error").ToBuffer();
		}
	}
}

LiMao::Data::DataPackage::TextDataPack LiMao::Modules::UserControl::UserControlModule::get_friend_requests_message(LiMao::Data::DataPackage::TextDataPack& pack)
{
	namespace us = LiMao::Modules::UserControl;
	TextPackWithLogin text_pack(pack.ToBuffer().ToString());
	if (!this->check_token(text_pack.uid, text_pack.token))
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 1, "", "Invalid token").ToBuffer();
	}
	try
	{
		us::User user;
		user.uid = text_pack.uid;
		auto requests = user.GetFriendRequest();
		TextPackWithLogin pack(text_pack.ID(), text_pack.uid, 0, text_pack.token, "Success");
		pack.row_json_obj.AddEmptySubObject("Data");
		pack.row_json_obj["Data"].AddEmptySubArray("RequestList");
		for (const auto& it : requests)
		{
			neb::CJsonObject obj;
			obj.Add("RequestUID", std::to_string(it.first));
			obj.Add("Message", it.second);
			pack.row_json_obj["Data"]["RequestList"].Add(obj);
		}
		return pack.ToBuffer();
	}
	catch (const UserControlException& ex)
	{
		if (ex.is_normal_exception)
		{
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, ex.what()).ToBuffer();
		}
		else
		{
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, "Server error").ToBuffer();
		}
	}
}

LiMao::Data::DataPackage::TextDataPack LiMao::Modules::UserControl::UserControlModule::agree_friend_request_message(LiMao::Data::DataPackage::TextDataPack& pack)
{
	TextPackWithLogin text_pack(pack.ToBuffer().ToString());
	if (!this->check_token(text_pack.uid, text_pack.token))
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 1, "", "Invalid token").ToBuffer();
	}
	namespace us = LiMao::Modules::UserControl;
	uint64_t allow_uid = 0;
	std::stringstream(text_pack.row_json_obj["Data"]("AgreeUID")) >> allow_uid;
	bool is_agree;
	if (!allow_uid)
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 2, text_pack.token, "Invalid agree uid").ToBuffer();
	}
	if (!text_pack.row_json_obj["Data"].Get("Agree", is_agree))
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 4, text_pack.token, "Invalid agree").ToBuffer();
	}
	try
	{
		us::User user;
		user.uid = text_pack.uid;
		auto requests = user.GetFriendRequest();
		for (const auto& it : requests)
		{
			if (it.first == allow_uid)
			{
				if (is_agree) user.AddFriend(allow_uid);
				user.RemoveFriendRequest(allow_uid);
				return TextPackWithLogin(text_pack.ID(), text_pack.uid, 0, text_pack.token, "Success").ToBuffer();
			}
		}
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 3, text_pack.token, "AgreeUID not requset friend").ToBuffer();
	}
	catch (const UserControlException& ex)
	{
		if (ex.is_normal_exception)
		{
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, ex.what()).ToBuffer();
		}
		else
		{
			LiMao::Service::Logger::LogError("User control module: Agree friend request message error:%s", ex.what());
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, "Server error").ToBuffer();
		}
	}
}

LiMao::Data::DataPackage::TextDataPack LiMao::Modules::UserControl::UserControlModule::get_username_with_uid(LiMao::Data::DataPackage::TextDataPack& pack)
{
	TextPackWithLogin text_pack(pack.ToBuffer().ToString());
	if (!this->check_token(text_pack.uid, text_pack.token))
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 1, "", "Invalid token").ToBuffer();
	}
	namespace us = LiMao::Modules::UserControl;
	uint64_t get_uid = 0;
	std::stringstream(text_pack.row_json_obj["Data"]("UID")) >> get_uid;
	try
	{
		std::string username = User::GetUserName(get_uid);
		neb::CJsonObject data;
		data.Add("UID", std::to_string(get_uid));
		data.Add("UserName", username);
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 0, text_pack.token, "Success", data).ToBuffer();
	}
	catch (const UserControlException& ex)
	{
		if (ex.is_normal_exception)
		{
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, ex.what()).ToBuffer();
		}
		else
		{
			LiMao::Service::Logger::LogError("User control module: Get username with uid error:%s", ex.what());
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, "Server error").ToBuffer();
		}
	}
}

LiMao::Data::DataPackage::TextDataPack LiMao::Modules::UserControl::UserControlModule::send_message_to_friend(LiMao::Data::DataPackage::TextDataPack& pack)
{
	//检查TOKDN
	TextPackWithLogin text_pack(pack.ToBuffer().ToString());
	if (!this->check_token(text_pack.uid, text_pack.token))
	{
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 1, "", "Invalid token").ToBuffer();
	}
	uint64_t friend_uid;
	std::stringstream(text_pack.row_json_obj["Data"]("FriendUID"))>>friend_uid;
	if (!friend_uid) return TextPackWithLogin(text_pack.ID(), text_pack.uid, 2, text_pack.token, "Invalid friend uid");
	if (text_pack.row_json_obj["Data"]("Message") == "") return TextPackWithLogin(text_pack.ID(), text_pack.uid, 4, text_pack.token, "Message is null");
	if (!LiMao::Modules::UserControl::Sessions::Session::Message::IsTypeSupported(text_pack.row_json_obj["Data"]("MessageType")))
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, 4, text_pack.token, "Message type unsupported");
	try
	{
		if (!User::CheckUserExist(friend_uid))
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, 3, text_pack.token, "Target absent").ToBuffer();
		User user;
		user.uid = text_pack.uid;
		auto friend_list = user.GetFriendList();
		if (std::find(friend_list.begin(), friend_list.end(), friend_uid) == friend_list.end())
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, 4, text_pack.token, "Target is not friend").ToBuffer();
	}
	catch (const UserControlException& ex)
	{
		if (ex.is_normal_exception)
		{
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, ex.what()).ToBuffer();
		}
		else
		{
			LiMao::Service::Logger::LogError("User control module: Get username with uid error:%s", ex.what());
			return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, "Server error").ToBuffer();
		}
	}
	//检查与好友是否存在已有会话
	try
	{
		LiMao::Modules::UserControl::Sessions::FriendsSessions self_session_table(text_pack.uid);
		LiMao::Modules::UserControl::Sessions::FriendsSessions friend_session_table(friend_uid);
		if (!self_session_table.IsFriendSessionExist(friend_uid))
		{
			auto session_id = LiMao::ID::UUID::GenerateTimeSafe();
			LiMao::Modules::UserControl::Sessions::Session::CreateSession(session_id).AddMembers({ text_pack.uid,friend_uid });
			self_session_table.AddFriendSession(friend_uid, session_id);
			friend_session_table.AddFriendSession(text_pack.uid, session_id);
		}
		LiMao::Modules::UserControl::Sessions::Session session(self_session_table.GetFriendSession(friend_uid));
		session.AddMessage(std::time(nullptr), text_pack.uid, text_pack.row_json_obj["Data"]("MessageType"),
			RbsLib::Buffer(text_pack.row_json_obj["Data"]("Message"), true));
	}
	catch (const Sessions::SessionException& ex)
	{
		//LiMao::Service::Logger::LogError("User control module: Get username with uid error:%s", ex.what());
		return TextPackWithLogin(text_pack.ID(), text_pack.uid, -1, text_pack.token, "Server error").ToBuffer();
	}
	//尝试给好友直接发送
	std::shared_lock<std::shared_mutex> lock1(this->online_connections_mutex);
	std::shared_lock<std::shared_mutex> lock2(this->users_mutex);
	if (this->user_connections.find(friend_uid) != this->user_connections.end() &&
		this->users.find(friend_uid) != this->users.end())
	{
		neb::CJsonObject data;
		data.Add("SenderUID", std::to_string(text_pack.uid));
		data.Add("FromType", "friend");//因为201号消息一定是发给好友的
		data.Add("MessageType", text_pack.row_json_obj["Data"]("MessageType"));
		data.Add("Message", text_pack.row_json_obj["Data"]("Message"));
		auto se = TextPackWithLogin(202, friend_uid, 0, this->users.find(friend_uid)->second.token, "Success",data);
		try
		{
			this->user_connections.find(friend_uid)->second->Send(se.ToBuffer());
		}
		catch (const LiMao::Service::SafeNetworkException& ex)
		{
		}
	}
	lock1.unlock();
	lock2.unlock();
	//返回成功
	return TextPackWithLogin(text_pack.id, text_pack.uid, 0, text_pack.token, "Success").ToBuffer();
}

bool LiMao::Modules::UserControl::UserControlModule::OnLoad(const LiMao::ID::UUID& module_uuid)
{
	this->task_pool = new LiMao::Service::TaskPool;
	this->task_pool->Run([this]() {
		std::shared_lock<std::shared_mutex> lock(this->users_mutex);
		lock.unlock();
		while (!this->exit)
		{
			std::this_thread::sleep_for(std::chrono::seconds(10));
			//收集在线用户中token过期的用户
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
				//删除过期用户
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
	delete this->task_pool;
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
		case 102://Request friend
			info.sending_service.Send(info.safe_connection, this->request_friend(dynamic_cast<LiMao::Data::DataPackage::TextDataPack&>(data)).ToBuffer());
			break;
		case 103://Get friend requests list
			info.sending_service.Send(info.safe_connection, this->get_friend_requests_message(dynamic_cast<LiMao::Data::DataPackage::TextDataPack&>(data)).ToBuffer());
			break;
		case 104://Agree friend request
			info.sending_service.Send(info.safe_connection, this->agree_friend_request_message(dynamic_cast<LiMao::Data::DataPackage::TextDataPack&>(data)).ToBuffer());
			break;
		case 105:
			info.sending_service.Send(info.safe_connection, this->get_username_with_uid(dynamic_cast<LiMao::Data::DataPackage::TextDataPack&>(data)).ToBuffer());
			break;
		case 201:
			info.sending_service.Send(info.safe_connection, this->send_message_to_friend(dynamic_cast<LiMao::Data::DataPackage::TextDataPack&>(data)).ToBuffer());
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

LiMao::Modules::UserControl::TextPackWithLogin::TextPackWithLogin(int id, uint64_t uid, int state, const std::string& token, const std::string& message)
	:uid(uid), token(token), message(message), TextDataPack(id, state) {}

LiMao::Modules::UserControl::TextPackWithLogin::TextPackWithLogin(int id, uint64_t uid, int state, const std::string& token, const std::string& message, const neb::CJsonObject& data)
	:uid(uid), token(token), message(message), TextDataPack(id, state)
{
	this->row_json_obj.Add("Data",data);
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
	json.ReplaceAdd("UID", std::to_string(this->uid));
	return LiMao::Data::DataPackage::TextDataPack(json.ToFormattedString()).ToBuffer();
}
