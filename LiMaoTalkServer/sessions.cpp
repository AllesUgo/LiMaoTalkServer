#include "sessions.h"
#include <shared_mutex>
#include "logger.h"
#include <fmt/format.h>
#include <mutex>
#include "base64_cpp.h"
#include <sstream>
static std::shared_mutex sessions_rw_lock;
void LiMao::Modules::UserControl::Sessions::Session::CheckAndCreateSessionTable(const DataBase::SQLite&sqlite)
{
	std::unique_lock<std::shared_mutex> lock(sessions_rw_lock);
	if (false == sqlite.IsTableExist("sessions"))
	{
		//Create sessions table
		try
		{
			sqlite.Exec("create table sessions(session_id TEXT PRIMARY KEY NOT NULL,members TEXT NOT NULL);");
		}
		catch (const DataBase::DataBaseException& ex)
		{
			throw SessionException(std::string("Create sessions table failed:") + ex.what());
		}
	}
}
void LiMao::Modules::UserControl::Sessions::Session::ThrowIfDisable(void) const
{
	if (!this->is_enable)
	{
		throw SessionException("Invalid session");
	}
}
auto LiMao::Modules::UserControl::Sessions::Session::unparse_members_string(const std::string& str) -> std::vector<std::string>
{
	bool left;
	std::vector<std::string> res;
	std::string un;
	for (char ch : str)
	{
		switch (ch)
		{
		case '[':
			left = true;
			un.clear();
			continue;
		case ']':
			if (left != true)
				throw SessionException("Unparse members string failed");
			left = false;
			res.push_back(un);
			continue;
		default:
			if (left) un.push_back(ch);
			break;
		}
	}
	return res;
}
void LiMao::Modules::UserControl::Sessions::Session::CheckAndCreateSessionMessageTable(const LiMao::ID::UUID& session_uuid, const DataBase::SQLite& sqlite)
{
	std::string table_name = std::string("session_messages_") + session_uuid.ToString();
	std::unique_lock<std::shared_mutex> lock(sessions_rw_lock);
	if (!sqlite.IsTableExist(table_name))
	{
		sqlite.Exec(fmt::format("CREATE TABLE '{}' (time INTEGER NOT NULL,sender TEXT,message_type TEXT,message TEXT);",table_name));
	}
}
auto LiMao::Modules::UserControl::Sessions::Session::MessageTableName(void) const noexcept -> std::string
{
	return std::string("session_messages_") + session_uuid.ToString();
}
LiMao::Modules::UserControl::Sessions::Session::Session(const LiMao::ID::UUID& session):sqlite(DataBase::SQLite::Open(Config::ConfigManager::UserMessageDatabasePath().c_str()))
{
	this->CheckAndCreateSessionTable(this->sqlite);
	std::unique_lock<std::shared_mutex> lock(sessions_rw_lock);
	//Check session exist
	try
	{
		if (sqlite.Exec(fmt::format("select session_id from sessions where session_id=\"{}\";", session.ToString())).size() == 0)
		{
			//不存在该条目
			throw SessionException("Session unexist");
		}
	}
	catch (DataBase::DataBaseException& ex)
	{
		//可能是命令执行失败
		throw SessionException(std::string("Check session exist failed:") + ex.what());
	}
	this->session_uuid = session;
	this->is_enable = true;
}

auto LiMao::Modules::UserControl::Sessions::Session::CreateSession(const LiMao::ID::UUID& session) -> Session
{
	DataBase::SQLite sqlite = DataBase::SQLite::Open(LiMao::Config::ConfigManager::UserMessageDatabasePath().c_str());
	CheckAndCreateSessionTable(sqlite);
	std::unique_lock<std::shared_mutex> lock(sessions_rw_lock);
	try
	{
		if (sqlite.Exec(fmt::format("select session_id from sessions where session_id=\"{}\";", session.ToString())).size() == 0)
		{
			//不存在该条目,创建条目
			try
			{
				sqlite.Exec(fmt::format("insert into sessions values (\"{}\",\"\");",session.ToString()));
			}
			catch (const DataBase::DataBaseException& ex)
			{
				throw SessionException(std::string("Create session failed: ") + ex.what());
			}
		}
	}
	catch (const DataBase::DataBaseException& ex)
	{
		//可能是命令执行失败
		throw SessionException(std::string("Check session exist failed: ") + ex.what());
	}
	lock.unlock();
	return Session(session);
}

auto LiMao::Modules::UserControl::Sessions::Session::GetMembers(void) const -> std::vector<uint64_t>
{
	std::shared_lock<std::shared_mutex> lock(sessions_rw_lock);
	this->ThrowIfDisable();
	std::vector<uint64_t> members;
	try
	{
		auto res = this->sqlite.Exec(fmt::format("select members from sessions where session_id=\"{}\";", this->session_uuid.ToString()));
		lock.unlock();
		for (const auto& it : res["members"])
		{
			std::vector<std::string> members_strs = this->unparse_members_string(it);
			for (const std::string& member_str : members_strs)
				members.push_back(stoul(member_str));
		}
	}
	catch (const DataBase::DataBaseException& ex)
	{
		//Command execute failed
		throw SessionException(std::string("Get members failed when search database:") + ex.what());
	}
	return members;
}

void LiMao::Modules::UserControl::Sessions::Session::SetMembers(const std::vector<uint64_t>& members) const
{
	std::shared_lock<std::shared_mutex> lock(sessions_rw_lock);
	this->ThrowIfDisable();
	std::string members_str;
	for (const auto& it : members)
		members_str += (std::string("[")+std::to_string(it) + "]");
	try
	{
		this->sqlite.Exec(fmt::format("UPDATE sessions SET members='{}' WHERE session_id='{}';",members_str,this->session_uuid.ToString()));
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(std::string("Set session members failed: ") + ex.what());
	}
}

void LiMao::Modules::UserControl::Sessions::Session::AddMembers(const std::vector<uint64_t>& members) const
{
	std::shared_lock<std::shared_mutex> lock(sessions_rw_lock);
	this->ThrowIfDisable();
	std::string members_str;
	for (const auto& it : members)
		members_str += (std::string("[") + std::to_string(it) + "]");
	try
	{
		this->sqlite.Exec(fmt::format("UPDATE sessions SET members=members||'{}' WHERE session_id='{}';", members_str, this->session_uuid.ToString()));
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(std::string("Set session members failed: ") + ex.what());
	}
}

void LiMao::Modules::UserControl::Sessions::Session::DeleteThisSession(void)
{
	std::unique_lock<std::shared_mutex> lock(sessions_rw_lock);
	this->ThrowIfDisable();
	try
	{
		this->sqlite.Exec(fmt::format("DELETE FROM sessions WHERE session_id='{}';", this->session_uuid.ToString()));
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(std::string("Delete session failed: ") + ex.what());
	}
	this->is_enable = false;
	sqlite.Close();
}

void LiMao::Modules::UserControl::Sessions::Session::AddMessage(uint64_t time, uint64_t user_uid, const std::string& type, const RbsLib::IBuffer& buffer) const
{
	this->CheckAndCreateSessionMessageTable(this->session_uuid, this->sqlite);
	std::unique_lock<std::shared_mutex> lock(sessions_rw_lock);
	try
	{
		this->sqlite.Exec(fmt::format("INSERT INTO '{}' VALUES ({},'{}','{}','{}');",
			this->MessageTableName(),
			std::time(nullptr),
			user_uid,
			type,
			LiMao::Encryption::Base64::Encode(buffer)));
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw fmt::format("Add message into session message table '{}' failed :{}", this->MessageTableName(), ex.what());
	}
}

auto LiMao::Modules::UserControl::Sessions::Session::GetMessages(int64_t start, int64_t end) const -> std::vector<Message>
{
	
	try
	{
		std::map<std::string, std::vector<std::string>> res;
		if (start >= 0 && end < 0)
			res = this->sqlite.Exec(fmt::format("select * from '{0}' order by time limit ((select count(time) from '{0}')+1+({1})-{2}) offset {2};",
				this->MessageTableName(),
				end, start
			));
		else if (start >= 0 && end >= 0 && end >= start)
			res = this->sqlite.Exec(fmt::format("SELECT * FROM '{0}' order by time limit {1}-{2}+1 offset {2};",
				this->MessageTableName(),
				end, start
			));
		else if (start < 0 && end < 0 && start <= end)
			res = this->sqlite.Exec(fmt::format("SELECT * FROM '{0}' ORDER BY time LIMIT ({2})-({1})+1 OFFSET (SELECT count(time) FROM '{0}')+({1});",
				this->MessageTableName(),
				start, end
			));
		else throw SessionException("Get messages error: error range");
		uint64_t ret_size = res["time"].size();
		std::vector<Message> messages(ret_size);
		for (int i = 0; i < ret_size; ++i)
		{
			uint64_t time, sender;
			std::stringstream(res["time"][i]) >> time;
			std::stringstream(res["sender"][i]) >> sender;
			messages[i] = Message(time, sender,res["message_type"][i], LiMao::Encryption::Base64::Decode(res["message"][i]));
		}
		return messages;
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(std::string("Gett messages failed : ") + ex.what());
	}
}

LiMao::Modules::UserControl::Sessions::SessionException::SessionException(const std::string& reason):reason(reason){}

const char* LiMao::Modules::UserControl::Sessions::SessionException::what(void) const noexcept
{
	return this->reason.c_str();
}

void LiMao::Modules::UserControl::Sessions::FriendsSessions::CreateUserFriendsSessionsTable(uint64_t user_uuid)
{
	//Connect to database
	try
	{
		DataBase::SQLite sqlite = DataBase::SQLite::Open(LiMao::Config::ConfigManager::UserMessageDatabasePath());
		sqlite.Exec(fmt::format("CREATE TABLE '{}_friends_sessions' (friend_uuid TEXT PRIMARY KEY NOT NULL,session_id TEXT NOT NULL);", user_uuid));
	}
	catch (const DataBase::DataBaseException& ex)
	{
		//Mabe database operation failed
		throw SessionException(fmt::format("Create user: {} friends table failed:{}", user_uuid, ex.what()));
	}
}

LiMao::Modules::UserControl::Sessions::FriendsSessions::FriendsSessions(uint64_t user_uuid)
	:sqlite(DataBase::SQLite::Open(LiMao::Config::ConfigManager::UserMessageDatabasePath()))
{
	std::string session_table_name = std::to_string(user_uuid) + "_friends_sessions";
	//Check friends sessions table is exist
	try
	{
		if (!this->sqlite.IsTableExist(session_table_name))
		{
			//table absent, create table
			this->CreateUserFriendsSessionsTable(user_uuid);
		}
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(fmt::format("Open user({})'s friends sessions failed: {}", user_uuid, ex.what()));
	}
	this->user_uuid = user_uuid;
	this->session_table = session_table_name;
}

auto LiMao::Modules::UserControl::Sessions::FriendsSessions::GetFriendSession(uint64_t friend_uuid) const -> LiMao::ID::UUID
{
	try
	{
		auto res = this->sqlite.Exec(fmt::format("SELECT session_id FROM '{}' WHERE friend_uuid='{}';", this->session_table, friend_uuid));
		if (res.size() == 0) 
			throw SessionException(fmt::format("Get user:{}'s friend:{} session failed: session not found", this->user_uuid, friend_uuid));
		return res["session_id"].at(0);
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(fmt::format("Get user:{}'s friend:{} session failed: {}", this->user_uuid, friend_uuid,ex.what()));
	}
}

void LiMao::Modules::UserControl::Sessions::FriendsSessions::AddFriendSession(uint64_t friend_uuid, const LiMao::ID::UUID& session_id) const
{
	try
	{
		this->sqlite.Exec(fmt::format("INSERT INTO '{}' VALUES ('{}','{}');", this->session_table, friend_uuid, session_id.ToString()));
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(fmt::format("Add friend session {}:{} failed: {}", friend_uuid, session_id.ToString(), ex.what()));
	}
}

void LiMao::Modules::UserControl::Sessions::FriendsSessions::DeleteFriendSession(uint64_t friend_uuid) const
{
	try
	{
		this->sqlite.Exec(fmt::format("DELETE FROM '{}' WHERE friend_uuid='{}';", this->session_table, friend_uuid));
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(fmt::format("Delete friend session failed: {}", ex.what()));
	}
}

bool LiMao::Modules::UserControl::Sessions::FriendsSessions::IsFriendSessionExist(uint64_t friend_uid) const
{
	try
	{
		auto res = this->sqlite.Exec(fmt::format("SELECT session_id FROM '{}' WHERE friend_uuid='{}';", this->session_table, friend_uid));
		if (res.size() == 0)
			return false;
		return true;
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(fmt::format("Get user:{}'s friend:{} session failed: {}", this->user_uuid, friend_uid, ex.what()));
	}
}

LiMao::Modules::UserControl::Sessions::Session::Message::Message(uint64_t time, uint64_t user_uid,const std::string&type ,const RbsLib::IBuffer& buffer)
	:time(time), user_uid(user_uid),type(type), message_base64(LiMao::Encryption::Base64::Encode(buffer)) {}

LiMao::Modules::UserControl::Sessions::Session::Message::Message(uint64_t time, uint64_t user_uid)
	:time(time), user_uid(user_uid) {}

void LiMao::Modules::UserControl::Sessions::Session::Message::message(const RbsLib::IBuffer& buffer) noexcept
{
	this->message_base64 = LiMao::Encryption::Base64::Encode(buffer);
}

auto LiMao::Modules::UserControl::Sessions::Session::Message::message(void) const -> RbsLib::Buffer
{
	return LiMao::Encryption::Base64::Decode(this->message_base64);
}

auto LiMao::Modules::UserControl::Sessions::Session::Message::IsTypeSupported(const std::string type) noexcept -> bool
{
	if (type == "text") return true;
	return false;
}
