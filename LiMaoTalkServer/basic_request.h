#pragma once
#include "modules_loader.h"
namespace LiMao
{
	namespace Modules
	{
		class BasicRequest :public IModule
		{
			LiMao::ID::UUID uuid;
		public:
			bool OnLoad(const LiMao::ID::UUID&uuid) override;
			bool OnUnload(void) override;
			bool OnMessage(LiMao::Data::DataPackage::DataPack& data, ConnectionInformation& info) override;
		};
	}
}