#pragma once
#include <exception>
#include <string>
#include "BaseType.h"
#include <vector>
namespace RbsLib
{
	class BufferException :public std::exception
	{
	private:
		std::string error_reason;
	public:
		BufferException(const std::string& string) noexcept;
		const char* what(void) const noexcept override;
	};
	class IBuffer
	{
	public:
		virtual const void* Data(void)const noexcept = 0;
		virtual uint64_t GetSize(void)const noexcept = 0;
		virtual uint64_t GetLength(void)const noexcept = 0;
	};
	class IWritableBuffer:public IBuffer
	{
	public:
		virtual void SetData(const void* data, uint64_t data_size) = 0;
	};
	class Buffer :public IWritableBuffer
	{
	private:
		void* data_ptr;
		uint64_t size;
		uint64_t length;
	public:
		Buffer(uint64_t size);
		Buffer(const void* data, uint64_t data_size);
		Buffer(const Buffer& buffer);
		Buffer(Buffer&& buffer);
		Buffer(const std::string& str, bool zero = false);
		~Buffer(void);
		const Buffer& operator=(const Buffer& buffer) noexcept;
		const Buffer& operator=(Buffer&& buffer) noexcept;
		uint8_t& operator[](uint64_t index);
		uint8_t operator[](uint64_t index) const;
		const void* Data(void)const noexcept override;
		uint64_t GetSize(void)const noexcept override;
		uint64_t GetLength(void)const noexcept override;
		std::string ToString(void)const noexcept;
		template <typename T> T GetData(void)const
		{
			return *(T*)(this->data_ptr);
		}
		void SetData(const void* data, uint64_t data_size);
		void SetLength(uint64_t len);
		void Resize(uint64_t buffer_size);
		template <typename T> std::vector<T> AsArray(void)
		{
			return std::vector<T>(this->data_ptr, (char*)this->data_ptr + this->length);
		}
	};
	
}