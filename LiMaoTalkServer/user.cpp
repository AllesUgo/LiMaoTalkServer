#include "user.h"
#include "configuration_manager.h"
#include "FileIO.h"
#include "base64_cpp.h"
#include "aes_cpp.h"
#include "uuid.h"
#include <shared_mutex>

LiMao::Modules::UserControl::UserControlException::UserControlException(const std::string& reason, bool is_normal_ex) noexcept
	:reason(reason), is_normal_exception(is_normal_ex) {}

const char* LiMao::Modules::UserControl::UserControlException::what() const noexcept
{
	return this->reason.c_str();
}

LiMao::Modules::UserControl::User::User(void) noexcept
{
}

LiMao::Modules::UserControl::User::User(const std::string& token, std::uint64_t uid, const std::string& username) noexcept
	:token(token), uid(uid), username(username) {}

auto LiMao::Modules::UserControl::User::BasicInfoToJson() const -> std::string
{
	neb::CJsonObject json;
	json.Add("UID", this->uid);
	json.Add("UserName", this->username);
	json.Add("Password", this->password);
	return json.ToFormattedString();
}

bool LiMao::Modules::UserControl::User::CheckBasicInfoJson(const neb::CJsonObject& obj)
{
	if (obj.KeyExist("UID") == false || obj.GetValueType("UID") != cJSON_Int)
		return false;
	if (obj.KeyExist("UserName") == false || obj.GetValueType("UserName") != cJSON_String)
		return false;
	if (obj.KeyExist("Password") == false || obj.GetValueType("Password") != cJSON_String)
		return false;
	return true;
}


auto LiMao::Modules::UserControl::User::Create(std::uint64_t uid, const std::string& username, const std::string& password) -> User
{
	if (uid == 0)
		throw UserControlException("Invalid uid");
	if (username.empty())
		throw UserControlException("Username is empty", true);
	if (password.length() < 6)
		throw UserControlException("Password length less than 6", true);
	User user("", uid, username);
	user.password = password;
	RbsLib::Storage::StorageFile file(LiMao::Config::ConfigManager::UserDataPath());
	if (file.IsExist() == false)
	{
		if (!std::filesystem::create_directories(file.Path()))
			throw UserControlException("Cannot create user data directory");
	}
	if (file[std::to_string(uid)].IsExist())
		throw UserControlException("User already exists");
	auto mutex = LiMao::Config::GlobalFileRWLock::GetMutex(file[std::to_string(uid)]["user.json"].Path());
	mutex->lock();
	file.CreateDir(std::to_string(uid));
	auto fp = file[std::to_string(uid)]["user.json"].Open(RbsLib::Storage::FileIO::OpenMode::Write |
		RbsLib::Storage::FileIO::OpenMode::Replace,
		RbsLib::Storage::FileIO::SeekBase::begin);
	fp.Write(RbsLib::Buffer(user.BasicInfoToJson()));
	user.password.clear();
	return user;
}

auto LiMao::Modules::UserControl::User::Login(std::uint64_t uid, const std::string& password) -> User
{
	if (!uid)
	{
		throw UserControlException("Invalid uid", 1);
	}
	/*检查用户是否存在*/
	RbsLib::Storage::StorageFile file(LiMao::Config::ConfigManager::UserDataPath());
	if (file[std::to_string(uid)]["user.json"].IsExist() == false)
	{
		throw UserControlException("User not exists", 1);
	}
	auto mutex = LiMao::Config::GlobalFileRWLock::GetMutex(file[std::to_string(uid)]["user.json"].Path());
	mutex->lock();
	uint64_t file_size = file[std::to_string(uid)]["user.json"].GetFileSize();
	if (file_size > 1024 * 1024)
	{
		throw UserControlException(file[std::to_string(uid)]["user.json"].Path() +
			" User data file too large: " + std::to_string(file_size)
			+ " more than " + std::to_string(1024 * 1024));
	}
	auto fp = file[std::to_string(uid)]["user.json"].Open(RbsLib::Storage::FileIO::OpenMode::Read,
		RbsLib::Storage::FileIO::SeekBase::begin);
	auto buffer = fp.Read(file_size);
	std::unique_ptr<char[]> json_buffer(new char[buffer.GetLength() + 1]);
	memcpy(json_buffer.get(), buffer.Data(), buffer.GetLength());
	json_buffer[buffer.GetLength()] = 0;
	neb::CJsonObject json(json_buffer.get());
	if (false == CheckBasicInfoJson(json))
		throw UserControlException(file[std::to_string(uid)]["user.json"].Path() +
			" User data file broken");
	uint64_t temp_uid;
	if (false == json.Get("UID", temp_uid) || temp_uid != uid)
		throw UserControlException(file[std::to_string(uid)]["user.json"].Path() +
			" User data file broken");
	if (password != json("Password"))
		throw UserControlException("UID or password incorrect", 1);
	return User(LiMao::Encryption::Base64::Encode(RbsLib::Buffer(std::to_string(std::time(nullptr) + 2 * 60 * 60) + " " + LiMao::ID::UUID::Generate().ToString())), uid, json("UserName"));
}

bool LiMao::Modules::UserControl::User::CheckUserExist(std::uint64_t uid)
{
	RbsLib::Storage::StorageFile file(LiMao::Config::ConfigManager::UserDataPath());
	return file[std::to_string(uid)]["user.json"].IsExist();
}

auto LiMao::Modules::UserControl::User::GetUserDir(std::uint64_t uid) -> RbsLib::Storage::StorageFile
{
	if (!CheckUserExist(uid))
	{
		throw UserControlException("User not exists");
	}
	RbsLib::Storage::StorageFile file(LiMao::Config::ConfigManager::UserDataPath());
	return file[std::to_string(uid)];
}

std::string LiMao::Modules::UserControl::User::ReadTextFile(const RbsLib::Storage::StorageFile& file, uint64_t max_size)
{
	if (file.IsExist() == false)
	{
		throw UserControlException("File not exists");
	}
	auto mutex = LiMao::Config::GlobalFileRWLock::GetMutex(file.Path());
	std::shared_lock<std::shared_mutex> lock(*mutex);
	if (file.GetFileSize() > max_size)
	{
		throw UserControlException("File too large");
	}
	auto fp = file.Open(RbsLib::Storage::FileIO::OpenMode::Read, RbsLib::Storage::FileIO::SeekBase::begin);
	auto buffer = fp.Read(file.GetFileSize());
	std::unique_ptr<char[]> json_buffer(new char[buffer.GetLength() + 1]);
	memcpy(json_buffer.get(), buffer.Data(), buffer.GetLength());
	json_buffer[buffer.GetLength()] = 0;
	return std::string(json_buffer.get());
}

void LiMao::Modules::UserControl::User::ReplaceWriteTetxtFile(const RbsLib::Storage::StorageFile& file, const std::string& text)
{
	auto mutex = LiMao::Config::GlobalFileRWLock::GetMutex(file.Path());
	std::unique_lock<std::shared_mutex> lock(*mutex);
	auto fp = file.Open(RbsLib::Storage::FileIO::OpenMode::Write | RbsLib::Storage::FileIO::OpenMode::Replace,
		RbsLib::Storage::FileIO::SeekBase::begin);
	fp.Write(RbsLib::Buffer(text));
}


std::vector<uint64_t> LiMao::Modules::UserControl::User::GetFriendList() const
{
	if (this->uid == 0) throw UserControlException("Invalid uid");
	auto user_dir = GetUserDir(this->uid);
	auto mutex = LiMao::Config::GlobalFileRWLock::GetMutex(user_dir["friends.json"].Path());
	std::shared_lock<std::shared_mutex> lock(*mutex);
	if (user_dir["friends.json"].IsExist() == false)
	{
		return std::vector<uint64_t>();
	}
	auto fp = user_dir["friends.json"].Open(RbsLib::Storage::FileIO::OpenMode::Read,
		RbsLib::Storage::FileIO::SeekBase::begin);
	/*检查文件大小*/
	auto file_size = user_dir["friends.json"].GetFileSize();
	if (file_size > 1024 * 1024)
	{
		throw UserControlException(user_dir["friends.json"].Path() +
			" User friends file too large: " + std::to_string(file_size)
			+ " more than " + std::to_string(1024 * 1024));
	}
	auto buffer = fp.Read(file_size);
	lock.unlock();
	std::unique_ptr<char[]> json_buffer(new char[buffer.GetLength() + 1]);
	memcpy(json_buffer.get(), buffer.Data(), buffer.GetLength());
	json_buffer[buffer.GetLength()] = 0;
	neb::CJsonObject json(json_buffer.get());
	if (json.GetValueType("Friends") == cJSON_Array)
	{
		std::vector<std::uint64_t> friends(json["Friends"].GetArraySize());
		for (int i = 0; i < json["Friends"].GetArraySize(); ++i)
		{
			uint64_t temp_uid;
			json["Friends"].Get(i, temp_uid);
			friends[i] = temp_uid;
		}
		return friends;
	}
	else
	{
		return std::vector<uint64_t>();
	}
}

void LiMao::Modules::UserControl::User::AddFriend(std::uint64_t uid) const
{
	if (uid == 0)
	{
		throw UserControlException("Invalid uid");
	}
	//检查当前用户目录是否存在
	auto user_dir = GetUserDir(this->uid);
	//检查好友目录是否存在
	if (CheckUserExist(uid) == false)
	{
		throw UserControlException("User not exists", 1);
	}
	//检查好友是否已经存在
	auto friends = GetFriendList();
	for (auto& friend_uid : friends)
	{
		if (friend_uid == uid)
		{
			throw UserControlException("Friend already exists", 1);
		}
	}
	//检查好友是否已经添加自己
	auto friend_user = User("", uid, "");
	auto friend_friends = friend_user.GetFriendList();
	for (auto& friend_uid : friend_friends)
	{
		if (friend_uid == this->uid)
		{
			throw UserControlException("Friend list not async");
		}
	}
	//添加好友
	std::string this_friends_json = this->ReadTextFile(user_dir["friends.json"], 1024 * 1024);
	neb::CJsonObject this_friends(this_friends_json);
	if (this_friends.GetValueType("Friends") == cJSON_Array)
	{
		this_friends["Friends"].Add(uid);
	}
	else
	{
		if (this_friends.KeyExist("Friends") == false)
		{
			this_friends.AddEmptySubArray("Friends");
			this_friends.Add("Friends", uid);
		}
		else
		{
			throw UserControlException("Invalid friends.json: value type error");
		}
	}
	//读取好友的好友列表
	std::string f_friends_json = this->ReadTextFile(GetUserDir(friend_user.uid)["friends.json"], 1024 * 1024);
	neb::CJsonObject f_friends(f_friends_json);
	if (f_friends.GetValueType("Friends") == cJSON_Array)
	{
		f_friends["Friends"].Add(this->uid);
	}
	else
	{
		if (f_friends.KeyExist("Friends") == false)
		{
			f_friends.AddEmptySubArray("Friends");
			f_friends.Add("Friends", this->uid);
		}
		else
		{
			throw UserControlException("Invalid friends.json: value type error");
		}
	}
	//写入好友列表
	this->ReplaceWriteTetxtFile(user_dir["friends.json"], this_friends.ToString());
	this->ReplaceWriteTetxtFile(GetUserDir(friend_user.uid)["friends.json"], f_friends.ToString());
}

void LiMao::Modules::UserControl::User::SendFriendRequest(std::uint64_t uid, const std::string& message) const
{
	if (uid == 0) throw UserControlException("Invalid uid");
	//检查当前用户目录是否存在
	if (this->uid == 0) throw UserControlException("Invalid uid");
	if (CheckUserExist(this->uid) == false)
	{
		throw UserControlException("User not exists");
	}
	//检查好友目录是否存在
	if (CheckUserExist(uid) == false)
	{
		throw UserControlException("User not exists",1);
	}
	//检查好友是否已经存在
	auto friends = this->GetFriendList();
	for (auto& friend_uid : friends)
	{
		if (friend_uid == uid)
		{
			throw UserControlException("Friend already exists",1);
		}
	}
	//检查目标是否已有该用户的好友请求
	User friend_user("", uid, "");
	auto requests = friend_user.GetFriendRequest();
	for (auto& request : requests)
	{
		if (request.first == this->uid)
		{
			throw UserControlException("Friend request already sended",1);
		}
	}
	//添加好友请求
	//读取目标好友请求列表
	neb::CJsonObject json;
	try
	{
		std::string json_text=ReadTextFile(GetUserDir(friend_user.uid)["friend_request.json"], 1024 * 1024);
		json = neb::CJsonObject(json_text);
		//检查是否存在Requests
		if (json.KeyExist("Requests") == false)
		{
			json.AddEmptySubArray("Requests");
		}
	}
	catch (const UserControlException&)
	{
		json.AddEmptySubArray("Requests");
	}
	//添加好友请求
	neb::CJsonObject obj;
	obj.Add("UID",this->uid);
	obj.Add("Message", message);
	json["Requests"].Add(obj);
	//写入好友请求列表
	ReplaceWriteTetxtFile(GetUserDir(friend_user.uid)["friend_request.json"], json.ToFormattedString());
}

std::map<std::uint64_t, std::string> LiMao::Modules::UserControl::User::GetFriendRequest() const
{
	//检查当前用户目录是否存在
	if (this->uid == 0) throw UserControlException("Invalid uid");
	if (!CheckUserExist(this->uid)) throw UserControlException("User not exists");
	//读取当前用户的好友请求
	try
	{
		std::string list=ReadTextFile(this->GetUserDir(this->uid)["friend_request.json"], 1024 * 1024);
		neb::CJsonObject json(list);
		if (json.GetValueType("Requests") == cJSON_Array)
		{
			std::map<std::uint64_t, std::string> requests;
			for (int i = 0; i < json["Requests"].GetArraySize(); ++i)
			{
				std::uint64_t temp_uid;
				std::string temp_message;
				json["Requests"][i].Get("UID", temp_uid);
				json["Requests"][i].Get("Message", temp_message);
				requests[temp_uid] = temp_message;
			}
			return requests;
		}
		else
		{
			return std::map<std::uint64_t, std::string>();
		}
	}
	catch (const UserControlException&)
	{
		return std::map<std::uint64_t, std::string>();
	}
}

void LiMao::Modules::UserControl::User::RemoveFriendRequest(std::uint64_t uid) const
{
	//检查当前用户目录是否存在
	if (this->uid == 0) throw UserControlException("Invalid uid");
	if (!CheckUserExist(this->uid)) throw UserControlException("User not exists");
	//读取当前用户的好友请求
	try
	{
		std::string list = ReadTextFile(this->GetUserDir(this->uid)["friend_request.json"], 1024 * 1024);
		neb::CJsonObject json(list);
		if (json.GetValueType("Requests") == cJSON_Array)
		{
			for (int i = 0; i < json["Requests"].GetArraySize(); ++i)
			{
				std::uint64_t temp_uid;
				json["Requests"][i].Get("UID", temp_uid);
				if (temp_uid == uid)
				{
					json["Requests"].Delete(i);
					ReplaceWriteTetxtFile(this->GetUserDir(this->uid)["friend_request.json"], json.ToString());
					return;
				}
			}
		}
	}
	catch (const UserControlException&)
	{
		return;
	}
}

bool LiMao::Modules::UserControl::User::IsTokenAllow(void) const noexcept
{
	return IsTokenAllow(this->token);
}

bool LiMao::Modules::UserControl::User::IsTokenAllow(const std::string& token) noexcept
{
	try
	{
		std::stringstream stream(LiMao::Encryption::Base64::Decode(token).ToString());
		time_t token_time = 0;
		stream >> token_time;
		if (token_time < time(nullptr))
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	catch (const LiMao::Encryption::EncryptionException&)
	{
		return false;
	}
}
