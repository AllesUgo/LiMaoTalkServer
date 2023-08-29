#include "Network.h"
#include <iostream>
#include "configuration_manager.h"
#include "logger.h"
#include "message_queue.h"
#include <thread>
#include "task_pool.h"
#include "connection_pool.h"
#include <list>
#include "safe_network.h"
#include "logger.h"
#include "uuid.h"
#include "datapackage.h"
#include "modules_loader.h"
#include "sending_async.h"
#include "basic_request.h"
#include "file_rw_lock.h"
#include "FileIO.h"
#include "base64_cpp.h"
#include "online_user_collection.h"
class MainServer
{
private:
	std::string addr;
	int port;
public:
	void Init(const std::string&conf_path);/*Init server config*/
	void Start(void);/*启动服务器*/
	~MainServer()noexcept;
};

int main()
{
	LiMao::Config::ConfigManager::SetConfigPath("config.json");
	LiMao::Config::ConfigManager::LocalAddress("0.0.0.0");
	LiMao::Config::ConfigManager::LocalPort(12345);
	LiMao::Config::ConfigManager::UidPoolDir("uid_pool");
	LiMao::Config::ConfigManager::UserDataPath("user_pool");
	LiMao::Config::ConfigManager::SaveConfig();
	MainServer server;
	LiMao::Service::Logger::LogInfo("finished");
	//for (auto& it : uuid_lst) LiMao::Service::Logger::LogInfo(it.ToString().c_str());
	server.Init("config.json");
	server.Start();
	return 0;
}

void MainServer::Init(const std::string& conf_path)
{
	using namespace LiMao::Config;
	ConfigManager::SetConfigPath(conf_path);
	ConfigManager::ReadConfig();
	this->addr = ConfigManager::LocalAddress();
	this->port = ConfigManager::LocalPort();
	LiMao::Modules::ModulesManager::RegisterModule(new LiMao::Modules::BasicRequest);
	LiMao::Modules::ModulesManager::RegisterModule(new LiMao::Modules::UserControl::UserControlModule);
}

void MainServer::Start(void)
{
	RbsLib::Network::TCP::TCPServer server(this->port, this->addr);
	server.Listen();
	LiMao::ConnectionControl::ConnectionPool pool;
	LiMao::Service::TaskPool task_pool(5);
	LiMao::Service::SendingAsync sending_service(5);
	bool is_close=false;
	task_pool.Run([&is_close]() {
		while (1)
		{
			std::string str;
			std::cin >> str;
			if (str == "exit")
			{
				is_close = true;
				return;
			}
		}
		});
	pool.Add(server);
	while (1)
	{
		try
		{
			auto connections = pool.WaitReadableActive(1);
			if (connections.size() == 0) continue;
			for (auto connection : connections)
			{
				task_pool.Run([connection,&pool,&sending_service]() {
					try
					{
						LiMao::Service::SafeNetwork network(connection, 1024 * 1024);
						auto Buffer = network.Recv();
						if (Buffer.GetSize() != 0)
						{
							LiMao::Data::DataPackage::TextDataPack data(Buffer);
							LiMao::Service::Logger::LogInfo("Recv pack from %s, ID=%d,State=%d",connection.GetAddress().c_str(), data.ID(), data.state);
							LiMao::Modules::ModulesManager::DealMessage(data, LiMao::Modules::ConnectionInformation(network, sending_service));
						}
						pool.Add(connection);
					}
					catch (LiMao::ConnectionControl::ConnectionControlException const& ex)
					{
						LiMao::Modules::ModulesManager::DealConnectionClosed(connection);
						LiMao::Service::Logger::LogError(ex.what());
					}
					catch (LiMao::Service::SafeNetworkException const& ex)
					{
						LiMao::Modules::ModulesManager::DealConnectionClosed(connection);
						LiMao::Service::Logger::LogError(ex.what());
					}
					catch (std::exception const& ex)
					{
						LiMao::Modules::ModulesManager::DealConnectionClosed(connection);
						LiMao::Service::Logger::LogError(ex.what());
					}
					catch (...)
					{
						LiMao::Modules::ModulesManager::DealConnectionClosed(connection);
						LiMao::Service::Logger::LogError("Unknown error");
					}
					});
			}
		}
		catch (LiMao::ConnectionControl::ConnectionControlException const& ex)
		{
			if (is_close) break;
		}
	}
}

MainServer::~MainServer() noexcept
{
	LiMao::Modules::ModulesManager::UnloadAllModule();
}
