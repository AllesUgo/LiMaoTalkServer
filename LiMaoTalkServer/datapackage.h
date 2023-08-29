#pragma once
#include <cstdint>
#include <exception>
#include "Buffer.h"
#include "CJsonObject.h"
namespace LiMao
{
	namespace Data
	{
		namespace DataPackage
		{
			class DataPackException:public std::exception
			{
			private:
				std::string reason;
			public:
				DataPackException(const std::string& reason)noexcept;
				const char* what(void)const noexcept override;
			};
			class DataPack
			{
			public:
				virtual int32_t ID(void) const=0;
				virtual RbsLib::Buffer ToBuffer(void) const=0;
			};
			class TextDataPack :public DataPack
			{
			public:
				neb::CJsonObject row_json_obj;
				int id=0;
				int state=0;
				TextDataPack(const RbsLib::IBuffer& buffer);
				TextDataPack(const std::string& json_data);
				TextDataPack() noexcept;
				TextDataPack(int id, int state) noexcept;
				void Parse(const RbsLib::IBuffer& buffer);
				int32_t ID(void) const override;
				virtual RbsLib::Buffer ToBuffer(void) const;
			};
		}
	}
}