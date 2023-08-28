#include "safe_network.h"
#include <cstring>
#include <string>
LiMao::Service::SafeNetwork::SafeNetwork(const RbsLib::Network::TCP::TCPConnection& connection, unsigned int max_pack_size)
	:connection(connection), max_pack_size(max_pack_size) {}

void LiMao::Service::SafeNetwork::Send(const RbsLib::IBuffer& buffer) const
{
	uint32_t size = buffer.GetLength();
	if (size > this->max_pack_size) throw SafeNetworkException("Safe send pack too large");
	if (4 != this->connection.Send(&size, sizeof(size), MSG_NOSIGNAL))
	{
		throw SafeNetworkException("Safe send size failed");
	}
	if (size) this->connection.Send(buffer, MSG_NOSIGNAL);
}

RbsLib::Buffer LiMao::Service::SafeNetwork::Recv(void) const
{

	uint32_t size;
	int need_size = 4;
	RbsLib::Buffer size_buf(4);
	try
	{
		while (need_size)
		{
			this->connection.Recv(size_buf, MSG_NOSIGNAL);
			std::memcpy((char*)&size + 4 - need_size, size_buf.Data(), size_buf.GetLength());
			need_size -= size_buf.GetLength();
		}
	}
	catch (const RbsLib::Network::NetworkException&ex)
	{
		throw SafeNetworkException(std::string("Safe recv size failed:") + ex.what());
	}
	if (size > this->max_pack_size) throw SafeNetworkException(std::string("Safe recv pack too large:") + std::to_string(size) + ">" + std::to_string(this->max_pack_size));
	char* data = new char[size];
	need_size = size;
	RbsLib::Buffer buffer(size);
	try
	{
		while (need_size)
		{
			this->connection.Recv(buffer);
			std::memcpy(data+size-need_size, buffer.Data(), buffer.GetLength());
			need_size -= buffer.GetLength();
		}
		buffer.SetData(data, size);
	}
	catch (const RbsLib::Network::NetworkException&ex)
	{
		delete[]data;
		throw SafeNetworkException(std::string("Safe recv data failed:") + ex.what());
	}
	catch(...)
	{
		delete[]data;
		throw;
	}
	delete[]data;
	return buffer;
}

auto LiMao::Service::SafeNetwork::GetConnection(void) const -> RbsLib::Network::TCP::TCPConnection const&
{
	return this->connection;
}

LiMao::Service::SafeNetworkException::SafeNetworkException(const std::string& reason) noexcept
	:reason(reason) {}

const char* LiMao::Service::SafeNetworkException::what(void) const noexcept
{
	return this->reason.c_str();
}

