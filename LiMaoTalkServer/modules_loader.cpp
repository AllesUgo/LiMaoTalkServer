#include "modules_loader.h"
#include "logger.h"
static std::map<LiMao::ID::UUID, LiMao::Modules::IModule*> modules;
LiMao::ID::UUID LiMao::Modules::ModulesManager::RegisterModule(IModule* module)
{
    LiMao::ID::UUID uuid;
    uuid = LiMao::ID::UUID::Generate();
    if (true == module->OnLoad(uuid))
    {
        modules[uuid] = module;
    }
    else 
    {
        delete module;
        throw ModuleException("Module load failed");
	}
    return uuid;
}

bool LiMao::Modules::ModulesManager::UnregisterModule(const LiMao::ID::UUID& uuid)
{
    if (modules.find(uuid) != modules.end())
    {
        if (true == modules[uuid]->OnUnload())
        {
			modules.erase(uuid);
			return true;
		}
	}
	return false;
}

void LiMao::Modules::ModulesManager::DealMessage(LiMao::Data::DataPackage::DataPack& data,const ConnectionInformation& info)
{
    for (auto& module : modules)
    {
        auto i = info;
        if (true == module.second->OnMessage(data,i)) return;
	}
}

void LiMao::Modules::ModulesManager::DealConnectionClosed(const RbsLib::Network::TCP::TCPConnection&connection)
{
    for (auto& module : modules)
    {
        module.second->OnConnectionClosed(LiMao::Service::SafeNetwork(connection,1024*1024));
    }
}

void LiMao::Modules::ModulesManager::UnloadAllModule(void)
{
    for (auto module = modules.begin(); module != modules.end();)
    {
        if (true == module->second->OnUnload())
        {
			/*¿ÉÒÔÐ¶ÔØ*/
            delete module->second;
            module = modules.erase(module);
        }
        else
        {
            LiMao::Service::Logger::LogWarn("Can not unload module %s", module->first.ToString().c_str());
            ++module;
        }
    }
}

LiMao::Modules::ModuleException::ModuleException(std::string const& reason) noexcept
{
    this->reason = reason;
}

const char* LiMao::Modules::ModuleException::what(void) const noexcept
{
    return this->reason.c_str();
}

LiMao::Modules::ConnectionInformation::ConnectionInformation(const LiMao::Service::SafeNetwork& safe_connection, LiMao::Service::SendingAsync& sending_service)
    :safe_connection(safe_connection), sending_service(sending_service){}

bool LiMao::Modules::IModule::OnLoad(const LiMao::ID::UUID& module_uuid) { return true; }

bool LiMao::Modules::IModule::OnUnload(void) {return true; }

void LiMao::Modules::IModule::OnConnectionClosed(const LiMao::Service::SafeNetwork& safe_connection){}
