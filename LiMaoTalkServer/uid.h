#pragma once
#include <stdint.h>
#include "id_exception.h"
namespace LiMao::ID
{
	class UID
	{
	public:
		static uint64_t Generate(void);
	};
}