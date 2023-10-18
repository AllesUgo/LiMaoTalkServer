#include "sessions.h"
#include <shared_mutex>
#include "logger.h"
#include <fmt/format.h>
#include <mutex>
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

auto LiMao::Modules::UserControl::Sessions::Session::GetMembers(void) const -> std::vector<LiMao::ID::UUID>
{
	std::shared_lock<std::shared_mutex> lock(sessions_rw_lock);
	this->ThrowIfDisable();
	std::vector<LiMao::ID::UUID> members;
	try
	{
		auto res = this->sqlite.Exec(fmt::format("select members from sessions where session_id=\"{}\";", this->session_uuid.ToString()));
		lock.unlock();
		for (const auto& it : res["members"])
		{
			std::vector<std::string> members_strs = this->unparse_members_string(it);
			for (const std::string& member_str : members_strs)
				members.push_back(LiMao::ID::UUID(member_str));
		}
	}
	catch (const DataBase::DataBaseException& ex)
	{
		//Command execute failed
		throw SessionException(std::string("Get members failed when search database:") + ex.what());
	}
	return members;
}

void LiMao::Modules::UserControl::Sessions::Session::SetMembers(const std::vector<LiMao::ID::UUID>& members) const
{
	std::shared_lock<std::shared_mutex> lock(sessions_rw_lock);
	this->ThrowIfDisable();
	std::string members_str;
	for (const auto& it : members)
		members_str += (std::string("[")+it.ToString()+"]");
	try
	{
		this->sqlite.Exec(fmt::format("UPDATE sessions SET members='{}' WHERE session_id='{}';",members_str,this->session_uuid.ToString()));
	}
	catch (const DataBase::DataBaseException& ex)
	{
		throw SessionException(std::string("Set session members failed: ") + ex.what());
	}
}

void LiMao::Modules::UserControl::Sessions::Session::AddMembers(const std::vector<LiMao::ID::UUID>& members) const
{
	std::shared_lock<std::shared_mutex> lock(sessions_rw_lock);
	this->ThrowIfDisable();
	std::string members_str;
	for (const auto& it : members)
		members_str += (std::string("[") + it.ToString() + "]");
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

LiMao::Modules::UserControl::Sessions::SessionException::SessionException(const std::string& reason):reason(reason){}

const char* LiMao::Modules::UserControl::Sessions::SessionException::what(void) const noexcept
{
	return this->reason.c_str();
}
