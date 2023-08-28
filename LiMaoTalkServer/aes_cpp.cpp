#include "aes_cpp.h"
#include "aes.h"
#include <cstring>
#include <mutex>
#include <memory>
std::mutex mutex;

unsigned int LiMao::Encryption::AES::calc_align(unsigned int n, unsigned align) noexcept
{
	return ((n + align - 1) & (~(align - 1)));
}

RbsLib::Buffer LiMao::Encryption::AES::Encrypt(const RbsLib::IBuffer& buffer, const RbsLib::IBuffer& key)
{
	std::unique_lock<std::mutex> lock(mutex);
	uint8_t* w;
	uint8_t rkey[32] = { 0 };
	if (key.GetLength() <= 16)
	{
		w = aes_init(16);
		std::memcpy(rkey, key.Data(), 16);
	}
	else if (key.GetLength() <= 24)
	{
		w = aes_init(24);
		std::memcpy(rkey, key.Data(), 24);
	}
	else
	{
		w = aes_init(32);
		std::memcpy(rkey, key.Data(), 32);
	}
	aes_key_expansion(rkey, w);
	uint64_t size = buffer.GetLength(),result_size= calc_align(size, 16);
	std::unique_ptr<uint8_t[]> result(new uint8_t[result_size]);
	uint8_t* data = (uint8_t*)buffer.Data();
	uint8_t* result_ptr = result.get();
	while (size >= 16)
	{
		aes_cipher(data, result_ptr, w);
		data += 16;
		result_ptr += 16;
		size -= 16;
	}
	if (size > 0)
	{
		uint8_t tmp[16] = { 0 };
		std::memcpy(tmp, data, size);
		aes_cipher(tmp, result_ptr, w);
	}
	free(w);
	return RbsLib::Buffer(result.get(), result_size);
}

RbsLib::Buffer LiMao::Encryption::AES::Decrypt(const RbsLib::IBuffer& buffer, const RbsLib::IBuffer& key)
{
	std::unique_lock<std::mutex> lock(mutex);
	if (buffer.GetLength() % 16 != 0)
	{
		throw EncryptionException("buffer length is not a multiple of 16");
	}
	uint8_t* w;
	uint8_t rkey[32] = { 0 };
	if (key.GetLength() <= 16)
	{
		w = aes_init(16);
		std::memcpy(rkey, key.Data(), 16);
	}
	else if (key.GetLength() <= 24)
	{
		w = aes_init(24);
		std::memcpy(rkey, key.Data(), 24);
	}
	else
	{
		w = aes_init(32);
		std::memcpy(rkey, key.Data(), 32);
	}
	aes_key_expansion(rkey, w);
	uint64_t size = buffer.GetLength(), result_size = calc_align(size, 16);
	uint8_t* data = (uint8_t*)buffer.Data();
	std::unique_ptr<uint8_t[]> result(new uint8_t[result_size]);
	uint8_t* result_ptr = result.get();
	while (size >= 16)
	{
		aes_inv_cipher(data, result_ptr, w);
		data += 16;
		result_ptr += 16;
		size -= 16;
	}
	if (size > 0)
	{
		uint8_t tmp[16] = { 0 };
		std::memcpy(tmp, data, size);
		aes_inv_cipher(tmp, result_ptr, w);
	}
	free(w);
	return RbsLib::Buffer(result.get(), result_size);
}

LiMao::Encryption::EncryptionException::EncryptionException(const std::string& reason)
	: reason(reason) {}

const char* LiMao::Encryption::EncryptionException::what() const noexcept
{
	return this->reason.c_str();
}
