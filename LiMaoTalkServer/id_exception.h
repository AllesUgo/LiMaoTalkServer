#pragma once
#include <string>
#include <exception>
namespace LiMao::ID
{
	class IDException :public std::exception
	{
	private:
		std::string reason;
	public:
		IDException(std::string const& reason);
		const char* what(void)const noexcept override;
	};
}