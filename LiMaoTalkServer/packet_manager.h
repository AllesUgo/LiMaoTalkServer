#pragma once
#include "datapackage.h"
#include "message_queue.h"
namespace LiMao
{
	namespace Data
	{
		namespace DataPackage
		{
			class PackManager
			{
			private:
				LiMao::Service::MessageQueue <LiMao::Data::DataPackage::DataPack*> pack_queue;
			public:
				PackManager(uint64_t max_item_num = 0);
				int PeekNextMessageID(int time_out=-1);
				LiMao::Data::DataPackage::DataPack* PeekNextMessage(int time_out = -1);
				LiMao::Data::DataPackage::DataPack* GetNextMessage(int time_out = -1);
				~PackManager();
			};
		}
	}
}