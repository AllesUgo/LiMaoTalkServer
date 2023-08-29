#pragma once
#include <string>
#include <exception>
#include <cstdint>
#include <vector>
#include <map>
#include "safe_network.h"
#include "CJsonObject.h"
#include "Storage.h"

namespace LiMao::Modules::UserControl
{
	class User;
	class UserControlException
	{
	private:
		std::string reason;
		
	public:
		bool is_normal_exception;
		UserControlException(const std::string& reason,bool is_normal_error=false)noexcept;
		const char* what()const noexcept;
	};
	class User
	{
	private:
		
		User(const std::string&token, std::uint64_t uid, const std::string& username) noexcept;
	public:
		User(void)noexcept;
		std::string token;
		std::string password;
		std::uint64_t uid;
		std::string username;
		auto BasicInfoToJson()const -> std::string;
		static bool CheckBasicInfoJson(const neb::CJsonObject& obj);
		static auto Create(std::uint64_t uid, const std::string& username, const std::string& password) -> User;
		static auto Login(std::uint64_t uid, const std::string& password) -> User;
		static bool CheckUserExist(std::uint64_t uid);
		static auto GetUserDir(std::uint64_t uid) -> RbsLib::Storage::StorageFile;
		static std::string ReadTextFile(const RbsLib::Storage::StorageFile& file,uint64_t max_size);
		static void ReplaceWriteTetxtFile(const RbsLib::Storage::StorageFile& file, const std::string&text);
		std::vector<uint64_t> GetFriendList()const;
		void AddFriend(std::uint64_t uid)const;
		void SendFriendRequest(std::uint64_t uid, const std::string& message)const;
		std::map<std::uint64_t,std::string> GetFriendRequest()const;
		void RemoveFriendRequest(std::uint64_t uid)const;
		bool IsTokenAllow(void)const noexcept;
		static bool IsTokenAllow(const std::string& token)noexcept;
	};
}