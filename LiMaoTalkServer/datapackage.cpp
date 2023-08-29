#include "datapackage.h"
#include <cstring>
#include <string>
LiMao::Data::DataPackage::DataPackException::DataPackException(const std::string& reason) noexcept
	:reason(reason) {}

const char* LiMao::Data::DataPackage::DataPackException::what(void) const noexcept
{
	return this->reason.c_str();
}

LiMao::Data::DataPackage::TextDataPack::TextDataPack(const RbsLib::IBuffer& buffer)
{
	this->Parse(buffer);
}

LiMao::Data::DataPackage::TextDataPack::TextDataPack(const std::string& json_data)
{
	this->Parse(RbsLib::Buffer(json_data));
}

LiMao::Data::DataPackage::TextDataPack::TextDataPack() noexcept {}

void LiMao::Data::DataPackage::TextDataPack::Parse(const RbsLib::IBuffer& buffer)
{
	if (buffer.GetLength() == 0) throw DataPackException("Create text data pack failed,buffer size is zero");
	if (!this->row_json_obj.Parse(std::string((const char*)buffer.Data(),buffer.GetLength())))
	{
		throw DataPackException(std::string("Create text data pack failed,JSON format error:")+this->row_json_obj.GetErrMsg());
	}
	if (!this->row_json_obj.Get("ID", this->id))
	{
		throw DataPackException("Create text data pack failed,json not have key \"ID\" or this key is not int type");
	}
	if (!this->row_json_obj.Get("State", this->state))
	{
		throw DataPackException("Create text data pack failed,json not have key \"State\" or this key is not int type");
	}
}

int32_t LiMao::Data::DataPackage::TextDataPack::ID(void) const
{
	return this->id;
}

RbsLib::Buffer LiMao::Data::DataPackage::TextDataPack::ToBuffer(void) const
{
	auto json = this->row_json_obj;
	json.ReplaceAdd("ID", this->ID());
	json.ReplaceAdd("State", this->state);
	return RbsLib::Buffer(json.ToFormattedString());
}
