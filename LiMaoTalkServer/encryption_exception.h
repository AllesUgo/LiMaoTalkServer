#pragma once
#include <exception>
namespace LiMao::Encryption
{
	class EncryptionException : public std::exception
	{
	private:
		std::string reason;
	public:
		EncryptionException(const std::string& reason);
		const char* what() const noexcept override;
	};
}