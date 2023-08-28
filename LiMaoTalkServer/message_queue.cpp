#include "message_queue.h"

LiMao::Service::MessageQueueException::MessageQueueException(Type type, const std::string& reason) noexcept
{
	this->reason = reason;
	this->type = type;
}

const char* LiMao::Service::MessageQueueException::what(void) const noexcept
{
	return this->reason.c_str();
}

