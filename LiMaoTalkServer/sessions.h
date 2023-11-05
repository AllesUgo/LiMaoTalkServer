#pragma once
#include "uuid.h"
#include "DataBase.h"
#include "sqlite_cpp.h"
#include "configuration_manager.h"
#include <exception>
#include <stdint.h>
#include "Buffer.h"
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
		static void CheckAndCreateSessionMessageTable(const LiMao::ID::UUID& session_uuid,const DataBase::SQLite& sqlite);
		auto MessageTableName(void) const noexcept ->std::string;
	public:
		class Message
		{
		private:
			std::string message_base64;
		public:
			Message() = default;
			Message(uint64_t time, uint64_t user_uid,const std::string&type, const RbsLib::IBuffer& buffer);
			Message(uint64_t time, uint64_t user_uid);
			uint64_t time;
			uint64_t user_uid;
			std::string type;
			void message(const RbsLib::IBuffer& buffer)noexcept;
			auto message(void) const->RbsLib::Buffer;
			static auto IsTypeSupported(const std::string type)noexcept->bool;
		};
		Session(const LiMao::ID::UUID& session);
		static auto CreateSession(const LiMao::ID::UUID& session)->Session;
		auto GetMembers(void)const->std::vector<uint64_t>;
		void SetMembers(const std::vector<uint64_t>& members)const;
		void AddMembers(const std::vector<uint64_t>& members)const;
		void DeleteThisSession(void);
		void AddMessage(uint64_t time, uint64_t user_uid,const std::string& type ,const RbsLib::IBuffer& buffer) const;
		auto GetMessages(int64_t start=0,int64_t end=-1) const->std::vector<Message>;//>=0为正向索引，负数为反向索引
	};
	class FriendsSessions
	{
	private:
		uint64_t user_uuid;
		std::string session_table;
		DataBase::SQLite sqlite;
		static void CreateUserFriendsSessionsTable(uint64_t user_uid);
	public:
		FriendsSessions(uint64_t user_uid);/*It will be created if friends_table absent*/
		auto GetFriendSession(uint64_t friend_uid) const->LiMao::ID::UUID;
		void AddFriendSession(uint64_t friend_uid, const LiMao::ID::UUID& session_id)const;
		void DeleteFriendSession(uint64_t friend_uid) const;
		bool IsFriendSessionExist(uint64_t friend_uid) const;
	};
}