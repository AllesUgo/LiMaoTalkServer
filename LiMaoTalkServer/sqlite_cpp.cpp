#include <fmt/format.h>
#include "sqlite_cpp.h"
#include "DataBase.h"

auto LiMao::DataBase::SQLite::Open(const char* path) -> SQLite
{
    SQLite db;
    db.counter_mutex = nullptr;
    if (sqlite3_threadsafe()!=1)
        if (SQLITE_OK != sqlite3_config(SQLITE_CONFIG_SERIALIZED))
            throw DataBaseException("Change to serialized mode failed");
    if (0 != sqlite3_open(path, &db.m_db))
    {
        throw DataBaseException("Open database failed");
    }
    db.refCount = new int(1);
    db.counter_mutex=new std::shared_mutex();
    return db;
}

LiMao::DataBase::SQLite::SQLite(const SQLite& other)
{
    other.counter_mutex->lock();
    this->counter_mutex=other.counter_mutex;
    this->refCount=other.refCount;
    if (*this->refCount > 0)
    {
		++(*this->refCount);//引用计数自增
	}
    this->m_db = other.m_db;
	other.counter_mutex->unlock();
}

LiMao::DataBase::SQLite::SQLite(SQLite&& other) noexcept
    :m_db(other.m_db), refCount(other.refCount), counter_mutex(other.counter_mutex)
{
    other.refCount = nullptr;
    other.m_db = nullptr;
    other.counter_mutex = nullptr;
}

LiMao::DataBase::SQLite::~SQLite() noexcept
{
    this->Close();
}

auto LiMao::DataBase::SQLite::operator=(const SQLite& other) -> const SQLite&
{
    if (this == &other) return *this;
    if (other.counter_mutex)
        other.counter_mutex->lock();
    this->counter_mutex = other.counter_mutex;
    this->m_db = other.m_db;
    this->refCount = other.refCount;
    if (this->refCount) ++*this->refCount;
    if (this->counter_mutex) other.counter_mutex->unlock();
    return *this;
}

auto LiMao::DataBase::SQLite::operator=(SQLite&& other) noexcept -> SQLite&
{
    if (this == &other) return *this;
    if (other.counter_mutex) other.counter_mutex->lock();
    this->counter_mutex = other.counter_mutex;
    this->m_db = other.m_db;
    this->refCount = other.refCount;
    other.counter_mutex = nullptr;
    other.m_db = nullptr;
    other.refCount = nullptr;
    if (this->counter_mutex) this->counter_mutex->unlock();
    return *this;
}

void LiMao::DataBase::SQLite::Close(void) noexcept
{
    if (this->counter_mutex)
    {
        this->counter_mutex->lock();
        if (--*this->refCount <= 0)
        {
            //需要释放资源
            this->counter_mutex->unlock();
            delete this->counter_mutex;
            delete this->refCount;
            sqlite3_close(this->m_db);
        }
        else this->counter_mutex->unlock();
        this->counter_mutex=nullptr;
    }
}
#include <stdio.h>
auto LiMao::DataBase::SQLite::Exec(const std::string& cmd) const ->std::map<std::string, std::vector<std::string>>
{
    if (!this->counter_mutex) throw DataBaseException("Database not open");
    std::map<std::string, std::vector<std::string>> ret;
    char* err_msg=nullptr;
    if (sqlite3_exec(this->m_db, cmd.c_str(),
        [](void* ret, int argc, char** argv, char** col_name)->int
        {
            std::map<std::string, std::vector<std::string>>* m = (std::map<std::string, std::vector<std::string>>*)ret;
            for (int i = 0; i < argc; ++i)
            {
                (*m)[col_name[i]].push_back(argv[i]?argv[i]:"");
            }
            return 0;
        },
        &ret,
        &err_msg
    )!=SQLITE_OK)
    {
        std::string errstring = std::string("SQLite command execute error:") + err_msg;
        sqlite3_free(err_msg);
        throw DataBaseException(errstring);
    }
    return ret;
}

auto LiMao::DataBase::SQLite::IsTableExist(const std::string& table_name) const -> bool
{
    auto res = this->Exec(fmt::format("SELECT name FROM sqlite_master WHERE type='table' and name='{}' order by name;",table_name));
    if ((*res.begin()).second.size() >= 1 && (*res.begin()).second[0] == table_name) return true;
    return false;
}


