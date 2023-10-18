#pragma once
#include "uuid.h"
#include "DataBase.h"
#include "sqlite_cpp.h"
#include "configuration_manager.h"
#include <exception>
namespace LiMao::Modules::UserControl::Sessions
{
	class SessionException : public std::exception
	{
	private:
		std::string reason;
	public:
		SessionException(const std::string& reason);
		const char* what(void) const noexcept override;
	};
	class Session
	{
	private:
		LiMao::ID::UUID session_uuid;
		DataBase::SQLite sqlite;
		bool is_enable;
		static void CheckAndCreateSessionTable(const DataBase::SQLite& sqlite);
		void ThrowIfDisable(void)const;
		static auto unparse_members_string(const std::string& str)->std::vector<std::string>;
	public:
		Session(const LiMao::ID::UUID& session);
		static auto CreateSession(const LiMao::ID::UUID& session)->Session;
		auto GetMembers(void)const->std::vector<LiMao::ID::UUID>;
		void SetMembers(const std::vector<LiMao::ID::UUID>& members)const;
		void AddMembers(const std::vector<LiMao::ID::UUID>& members)const;
		void DeleteThisSession(void);
	};
}