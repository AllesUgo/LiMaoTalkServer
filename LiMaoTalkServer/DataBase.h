#pragma once
#include <string>
#include <exception>
namespace LiMao::DataBase
{
	class DataBaseException : public std::exception
	{
	private:
		std::string m_msg;
	public:
		DataBaseException(const std::string& reason) noexcept;
		const char* what() const noexcept override;
	};
}