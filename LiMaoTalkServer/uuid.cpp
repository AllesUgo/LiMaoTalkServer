#include "uuid.h"



LiMao::ID::UUID::UUID() noexcept
{
	uuid_clear(this->uuid);
}

LiMao::ID::UUID::UUID(const std::string& uuid_str)
{
	if (uuid_parse(uuid_str.c_str(), this->uuid)) throw IDException("UUID string parse failed");
}

LiMao::ID::UUID LiMao::ID::UUID::Generate(void) noexcept
{

	LiMao::ID::UUID id;
	uuid_generate(id.uuid);
	return id;
}

LiMao::ID::UUID LiMao::ID::UUID::GenerateRandom(void) noexcept
{
	LiMao::ID::UUID id;
	uuid_generate_random(id.uuid);
	return id;
}

LiMao::ID::UUID LiMao::ID::UUID::GenerateTime(void) noexcept
{
	LiMao::ID::UUID id;
	uuid_generate_time(id.uuid);
	return id;
}

LiMao::ID::UUID LiMao::ID::UUID::GenerateTimeSafe(void) noexcept
{
	LiMao::ID::UUID id;
	uuid_generate_time_safe(id.uuid);
	return id;
}

auto LiMao::ID::UUID::ParseUUIDString(const std::string& uuid_string) -> UUID
{
	return UUID(uuid_string);
}

std::string LiMao::ID::UUID::ToString(void) const noexcept
{
	char str[512];
	uuid_unparse(this->uuid, str);
	std::string s;
	return std::string(str);
}

std::string LiMao::ID::UUID::ToUpperString(void) const noexcept
{
	char str[512];
	uuid_unparse_upper(this->uuid, str);
	return std::string(str);
}

std::string LiMao::ID::UUID::TpLowerString(void) const noexcept
{
	char str[512];
	uuid_unparse_lower(this->uuid, str);
	return std::string(str);
}

bool LiMao::ID::UUID::operator==(const UUID& uuid) const noexcept
{
	return !uuid_compare(this->uuid, uuid.uuid);
}

bool LiMao::ID::UUID::operator<(const UUID& uuid) const noexcept
{
	if (uuid_compare(this->uuid, uuid.uuid) < 0) return true;
	else return false;
}

bool LiMao::ID::UUID::operator>(const UUID& uuid) const noexcept
{
	if (uuid_compare(this->uuid, uuid.uuid) > 0) return true;
	else return false;
}

LiMao::ID::IDException::IDException(std::string const& reason)
	:reason(reason)
{
}

const char* LiMao::ID::IDException::what(void) const noexcept
{
	return this->reason.c_str();
}
