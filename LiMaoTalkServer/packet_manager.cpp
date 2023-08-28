#include "packet_manager.h"

LiMao::Data::DataPackage::PackManager::PackManager(uint64_t max_item_num):pack_queue(max_item_num)
{
}

int LiMao::Data::DataPackage::PackManager::PeekNextMessageID(int time_out)
{
	DataPack* p = this->pack_queue.PeekItem(time_out);
	return p->ID();
}

LiMao::Data::DataPackage::DataPack* LiMao::Data::DataPackage::PackManager::PeekNextMessage(int time_out)
{
	return this->pack_queue.PeekItem(time_out);
}

LiMao::Data::DataPackage::DataPack* LiMao::Data::DataPackage::PackManager::GetNextMessage(int time_out)
{
	return this->pack_queue.GetItem(time_out);
}

LiMao::Data::DataPackage::PackManager::~PackManager()
{
	try
	{
		while (1) delete this->pack_queue.GetItem(0);
	}
	catch (const LiMao::Service::MessageQueueException& ex)
	{
	}
}
