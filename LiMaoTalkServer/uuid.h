#pragma once
#include <uuid/uuid.h>
#include <string>
#include <exception>
#include "id_exception.h"
namespace LiMao
{
	namespace ID
	{
		
		class UUID
		{
		private:
			uuid_t uuid;
		public:
			UUID() noexcept;
			UUID(const std::string& uuid_str);
			static LiMao::ID::UUID Generate(void) noexcept;
			static LiMao::ID::UUID GenerateRandom(void) noexcept;
			static LiMao::ID::UUID GenerateTime(void) noexcept;
			static LiMao::ID::UUID GenerateTimeSafe(void) noexcept;
			static auto ParseUUIDString(const std::string& uuid_string) -> UUID;
			std::string ToString(void)const noexcept;
			std::string ToUpperString(void)const noexcept;
			std::string TpLowerString(void)const noexcept;
			bool operator==(const UUID& uuid) const noexcept;
			bool operator<(const UUID& uuid) const noexcept;
			bool operator>(const UUID& uuid) const noexcept;
		};
	}
}