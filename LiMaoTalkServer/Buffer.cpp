#include "Buffer.h"
#include <cstring>
RbsLib::BufferException::BufferException(const std::string& string)noexcept :error_reason(string) {}

const char* RbsLib::BufferException::what(void) const noexcept
{
	return error_reason.c_str();
}

RbsLib::Buffer::Buffer(uint64_t size)
{
	if (size == 0) throw RbsLib::BufferException("Buffer size can not be set with 0");
	else
	{
		this->data_ptr = new char[size];
		this->size = size;
		this->length = 0;
	}
}

RbsLib::Buffer::Buffer(const void* data, uint64_t data_size)
{
	if (data_size == 0) throw RbsLib::BufferException("Buffer size can not be set with 0");
	this->data_ptr = new char[data_size];
	this->size = data_size;
	this->length = data_size;
	memcpy(this->data_ptr, data, data_size);
}

RbsLib::Buffer::Buffer(const Buffer& buffer)
{
	this->data_ptr = new char[buffer.GetSize()];
	this->size = buffer.GetSize();
	this->length = buffer.GetLength();
	memcpy(this->data_ptr, buffer.Data(), buffer.length);
}

RbsLib::Buffer::Buffer(Buffer&& buffer)
{
	this->data_ptr = buffer.data_ptr;
	this->length = buffer.length;
	this->size = buffer.size;
	buffer.data_ptr=nullptr;
}

RbsLib::Buffer::Buffer(const std::string& str, bool zero)
{
	if (zero)
	{
		this->data_ptr = new char[str.length() + 1];
		strcpy((char*)this->data_ptr, str.c_str());
		this->length = this->size = str.length() + 1;
	}
	else
	{
		this->data_ptr = new char[str.length()];
		memcpy(this->data_ptr, str.c_str(), str.length());
		this->length = this->size = str.length();
	}
}

RbsLib::Buffer::~Buffer(void)
{
	delete[](char*)this->data_ptr;
}

const RbsLib::Buffer& RbsLib::Buffer::operator=(const Buffer& buffer) noexcept
{
	delete[](char*)this->data_ptr;
	this->data_ptr = new char[buffer.GetSize()];
	this->size = buffer.GetSize();
	this->length = buffer.GetLength();
	memcpy(this->data_ptr, buffer.Data(), buffer.length);
	return *this;
}

const RbsLib::Buffer& RbsLib::Buffer::operator=(Buffer&& buffer) noexcept
{
	if (this == &buffer) return *this;
	this->data_ptr = buffer.data_ptr;
	this->size = buffer.GetSize();
	this->length = buffer.GetLength();
	buffer.data_ptr = nullptr;
	return *this;
}

uint8_t& RbsLib::Buffer::operator[](uint64_t index)
{
	if (index < this->GetSize()) return *(uint8_t*)((uint8_t*)this->data_ptr + index);
	else throw RbsLib::BufferException("Buffer index out of range");
}

uint8_t RbsLib::Buffer::operator[](uint64_t index) const
{
	if (index < this->GetSize()) return *(uint8_t*)((uint8_t*)this->data_ptr + index);
	else throw RbsLib::BufferException("Buffer index out of range");
}

const void* RbsLib::Buffer::Data(void) const noexcept
{
	return this->data_ptr;
}

uint64_t RbsLib::Buffer::GetSize(void) const noexcept
{
	return this->size;
}

uint64_t RbsLib::Buffer::GetLength(void) const noexcept
{
	return this->length;
}

std::string RbsLib::Buffer::ToString(void) const noexcept
{
	return std::string((const char*)this->Data(), this->GetLength());
}

void RbsLib::Buffer::SetData(const void* data, uint64_t data_size)
{
	if (data_size > this->size) throw BufferException("data_size > buffer_size");
	else
	{
		memcpy(this->data_ptr, data, data_size);
		this->length = data_size;
	}
}

void RbsLib::Buffer::SetLength(uint64_t len)
{
	if (len <= this->GetSize()) this->length = len;
}

void RbsLib::Buffer::Resize(uint64_t buffer_size)
{
	if (buffer_size == 0) throw RbsLib::BufferException("Buffer size can not be set with 0");
	void* data = new char[buffer_size];
	if (buffer_size >= this->length)
	{
		memcpy(data, this->data_ptr, this->length);
		delete[](char*)this->data_ptr;
		this->data_ptr = data;
		this->size = buffer_size;
	}
	else
	{
		memcpy(data, this->data_ptr, this->size);
		delete[](char*)this->data_ptr;
		this->data_ptr = data;
		this->length = this->size = buffer_size;
	}
}

