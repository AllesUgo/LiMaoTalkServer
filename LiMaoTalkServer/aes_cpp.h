#pragma once
#include <exception>
#include <string>
#include "Buffer.h"
#include "encryption_exception.h"
namespace LiMao::Encryption
{
	
	
	class AES
	{
	private:
		static unsigned int calc_align(unsigned int n, unsigned align) noexcept;
	public:
		static RbsLib::Buffer Encrypt(const RbsLib::IBuffer& buffer, const RbsLib::IBuffer& key);
		static RbsLib::Buffer Decrypt(const RbsLib::IBuffer& buffer, const RbsLib::IBuffer& key);
	};
}