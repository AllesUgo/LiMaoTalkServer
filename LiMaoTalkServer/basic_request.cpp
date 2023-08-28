#include "basic_request.h"
#include "datapackage.h"
#include "logger.h"
bool LiMao::Modules::BasicRequest::OnLoad(const LiMao::ID::UUID&uuid)
{
    this->uuid = uuid;
    LiMao::Service::Logger::LogInfo("Basic request module loaded,uuid = %s",uuid.ToString().c_str());
    return true;
}

bool LiMao::Modules::BasicRequest::OnUnload(void)
{
    LiMao::Service::Logger::LogInfo("Basic request module unloaded,uuid = %s",uuid.ToString().c_str()); 
    return true;
}

bool LiMao::Modules::BasicRequest::OnMessage(LiMao::Data::DataPackage::DataPack& data, ConnectionInformation& info)
{
    try
    {
        auto& d=dynamic_cast<LiMao::Data::DataPackage::TextDataPack&>(data);
        if (d.ID() == 0 && d.state == 0)
            info.sending_service.Send(info.safe_connection, d.ToBuffer());
        else return false;
        return true;
    }
    catch (const std::exception& ex)
    {
        LiMao::Service::Logger::LogError(ex.what());
    }
    return false;
}
