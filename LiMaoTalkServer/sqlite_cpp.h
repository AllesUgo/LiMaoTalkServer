#pragma once
#include <sqlite3.h>
#include <shared_mutex>
#include <vector>
#include <map>

namespace LiMao::DataBase
{
	
	class SQLite
	{
	private:
		sqlite3* m_db = nullptr;
		int *refCount = nullptr;
		std::shared_mutex* counter_mutex;
		SQLite() = default;
	public:
		static auto Open(const char* path) -> SQLite;
		SQLite(const SQLite& other);
		SQLite(SQLite&& other) noexcept;
		~SQLite()noexcept;
		auto operator=(const SQLite& other) -> const SQLite&;
		auto operator=(SQLite&& other) noexcept -> SQLite&;
		void Close(void)noexcept;
		auto Exec(const std::string& cmd) -> std::map<std::string,std::vector<std::string>>;
		auto IsTableExist(const std::string& table_name) -> bool;
	};
}