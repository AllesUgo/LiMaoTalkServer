#pragma once
#include <string>
#include "Buffer.h"
#include "encryption_exception.h"
namespace LiMao::Encryption
{
	class Base64
	{
	public:
		static std::string Encode(RbsLib::IBuffer const&buffer)noexcept;
		static RbsLib::Buffer Decode(const std::string&str);
	};
}