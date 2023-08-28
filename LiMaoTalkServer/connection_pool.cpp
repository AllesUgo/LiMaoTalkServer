#include "connection_pool.h"
#include <cassert>
LiMao::ConnectionControl::ConnectionPool::ConnectionPool()
{
	//Create loop pipe to interrupt select
	if (pipe(this->loop_fd) == -1) 
		throw LiMao::ConnectionControl::ConnectionControlException("Create loop pipe failed");
}

LiMao::ConnectionControl::ConnectionPool::~ConnectionPool() noexcept
{
	close(this->loop_fd[0]);
	close(this->loop_fd[1]);
}

void LiMao::ConnectionControl::ConnectionPool::Add(const RbsLib::Network::TCP::TCPConnection& connection)
{
	std::unique_lock<std::mutex> lock(this->mutex);
	this->connections.push_back(connection);
	this->AlarmWaitReadableActive();
}

void LiMao::ConnectionControl::ConnectionPool::Add(const RbsLib::Network::TCP::TCPServer& server)
{
	std::unique_lock<std::mutex> lock(this->mutex);
	this->servers.push_back(server);
	this->AlarmWaitReadableActive();
}

std::list<RbsLib::Network::TCP::TCPConnection> LiMao::ConnectionControl::ConnectionPool::WaitReadableActive(int32_t timeout)
{
	fd_set sets;
	FD_ZERO(&sets);
	int i = 0;
	int max_fd=0;
	std::unique_lock<std::mutex> lock(this->mutex);
	if (this->connections.size() == 0&&this->servers.size()==0) throw LiMao::ConnectionControl::ConnectionControlException("No connection in pool");
	for (auto& it : this->connections)
	{
		if (it.GetSocket() > max_fd) max_fd = it.GetSocket();
		FD_SET(it.GetSocket(), &sets);
	}
	for (auto& it : this->servers)
	{
		if (it.GetSocket() > max_fd) max_fd = it.GetSocket();
		FD_SET(it.GetSocket(), &sets);
	}
	lock.unlock();
	/*Add the loop pipe to select*/
	FD_SET(this->loop_fd[0], &sets);
	if (max_fd<this->loop_fd[0]) max_fd = this->loop_fd[0];
	
	if (timeout >= 0)
	{
		timeval t;
		t.tv_sec = timeout;
		t.tv_usec = 0;
		i = select(max_fd + 1, &sets, nullptr, nullptr, &t);

	}
	else
	{
		i = select(max_fd + 1, &sets, nullptr, nullptr, nullptr);
	}
	if (i < 0) 
		throw LiMao::ConnectionControl::ConnectionControlException("Select failed");
	else if (i==0)
		throw LiMao::ConnectionControl::ConnectionControlException("Select timeout");
	if (FD_ISSET(this->loop_fd[0], &sets))
	{
		char buf;
		if (1 != read(this->loop_fd[0], &buf, 1))/*Read one byte to reset pipe*/
		{
			abort();
		}
		if (buf != 1) throw LiMao::ConnectionControl::ConnectionControlException("Loop pipe error");
	}
	std::list < RbsLib::Network::TCP::TCPConnection> lst;
	lock.lock();
	this->connections.remove_if([&lst,&sets](RbsLib::Network::TCP::TCPConnection& it)
		{
			if (FD_ISSET(it.GetSocket(), &sets))
			{
				lst.push_back(it);
				return true;
			}
			return false;
		});
	for (auto& it : this->servers)
	{
		if (FD_ISSET(it.GetSocket(), &sets))
		{
			lock.unlock();
			this->connections.push_back(it.Accept());
			lock.lock();
		}
	}
	/*Check out loop pipe*/
	
	return lst;
}

void LiMao::ConnectionControl::ConnectionPool::Remove(const RbsLib::Network::TCP::TCPConnection& connection)
{
	this->connections.remove_if([&connection](auto& it) {return connection == it; });
}

void LiMao::ConnectionControl::ConnectionPool::Remove(SOCKET sock)
{
	this->connections.remove_if([sock](auto& it) {return it.GetSocket() == sock; });
}

void LiMao::ConnectionControl::ConnectionPool::AlarmWaitReadableActive(void)
{
	char buf = 1;
	if (1 != write(this->loop_fd[1], &buf, 1))
	{
		abort();
	}
}
