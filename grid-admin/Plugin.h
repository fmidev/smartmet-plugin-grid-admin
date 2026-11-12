#pragma once
#include <spine/SmartMetPlugin.h>
#include <grid-files/common/ConfigurationFile.h>
#include <grid-content/contentServer/http/server/ServerInterface.h>
#include <json/json.h>
#include <cstdint>
#include <map>
#include <mutex>
#include <ctime>
#include <memory>
#include <string>
#include "Browser.h"

namespace SmartMet
{
namespace Engine
{
namespace Grid
{
class Engine;
}
}
}
namespace SmartMet
{
namespace SessionManagement
{
class SessionInfo;
}
}
namespace SmartMet
{
namespace Spine
{
class Reactor;
}
}
namespace SmartMet
{
namespace Spine
{
namespace HTTP
{
class Request;
class Response;
}
}
}



namespace SmartMet
{

namespace Plugin
{
namespace GridAdmin
{


// ====================================================================================
/*! \brief SmartMet Server plugin that exposes the Content Server HTTP API and a
 *  simple HTML admin browser for grid server state.
 *
 *  Registered as a private (admin) plugin at `/grid-admin`.  Requests with a `method`
 *  query parameter are forwarded to the Content Server HTTP API; requests with a `page`
 *  parameter are handled by the Browser UI (login, engines, plugins, software, and
 *  grid-engine-specific pages delegated to the Grid Engine browser).
 *
 *  Holds two ContentServer::HTTP::ServerInterface instances so that API requests can be
 *  routed to either the direct backend (`itsMessageProcessor1`) or the Grid Engine's
 *  in-memory cache (`itsMessageProcessor2`) via `source=engine`. */
// ====================================================================================

class Plugin : public SmartMetPlugin
{
  public:

                        Plugin() = delete;
                        Plugin(Spine::Reactor* theReactor, const char* theConfig);
                        Plugin(const Plugin& other) = delete;
    Plugin&             operator=(const Plugin& other) = delete;
    ~Plugin() override;

    const std::string&  getPluginName() const override;
    int                 getRequiredAPIVersion() const override;
    bool                isAdminQuery(const Spine::HTTP::Request& theRequest) const override;

  protected:

    void                init() override;
    void                shutdown() override;
    void                requestHandler(Spine::Reactor& theReactor,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse) override;

  private:
    bool                request(const Spine::Reactor& theReactor,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                apiRequest(const Spine::Reactor &theReactor,const Spine::HTTP::Request &theRequest,Spine::HTTP::Response &theResponse);

    // The new user interface (Ui.cpp)
    int                 ui_request(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse,const std::string& path);
    int                 ui_file(const std::string& name,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    int                 ui_api(const std::string& name,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    Json::Value         api_session(SessionManagement::SessionInfo& session);
    Json::Value         api_overview();
    Json::Value         api_producers(const Spine::HTTP::Request& theRequest);
    Json::Value         api_generations(const Spine::HTTP::Request& theRequest);
    Json::Value         api_files(const Spine::HTTP::Request& theRequest);
    Json::Value         api_contents(const Spine::HTTP::Request& theRequest);
    Json::Value         api_config();
    Json::Value         api_configfile(const Spine::HTTP::Request& theRequest);
    Json::Value         api_logs();
    Json::Value         api_log(const Spine::HTTP::Request& theRequest);
    Json::Value         api_software();
    Json::Value         api_modify(const Spine::HTTP::Request& theRequest,SessionManagement::SessionInfo& session);
    Json::Value         api_logcontrol(const Spine::HTTP::Request& theRequest,SessionManagement::SessionInfo& session);
    bool                canModify(SessionManagement::SessionInfo& session);
    bool                canControlLogs(SessionManagement::SessionInfo& session);
    std::string         permissionNote(SessionManagement::SessionInfo& session,
                                       std::uint64_t flag,
                                       const std::string& flagName,
                                       const std::string& action);
    Json::Value         sourcesJson();
    ContentServer_sptr  requestedContentServer(const Spine::HTTP::Request& theRequest);

    /*! \brief A file of the user interface kept in memory, read again when it changes. */
    struct UiFile
    {
      time_t            modificationTime = 0;
      std::string       content;
      std::string       etag;
    };


    const std::string   itsModuleName;                      //!< Plugin name returned by getPluginName().
    Spine::Reactor*     itsReactor;                         //!< Back-pointer to the server reactor (not owned).
    ConfigurationFile   itsConfigurationFile;               //!< Parsed libconfig configuration.
    std::string         itsContentServerType;               //!< Content server backend type: "redis", "postgresql", "http", or "corba".
    std::string         itsContentServerRedisAddress;       //!< Redis primary host address.
    int                 itsContentServerRedisPort;          //!< Redis primary port.
    std::string         itsContentServerRedisTablePrefix;   //!< Redis key prefix for this content source.
    std::string         itsContentServerRedisSecondaryAddress; //!< Redis secondary (replica) host address.
    int                 itsContentServerRedisSecondaryPort; //!< Redis secondary port (0 = disabled).
    bool                itsContentServerRedisLockEnabled;   //!< Whether Redis distributed locking is enabled.
    std::string         itsContentServerRedisPassword;      //!< Redis password for default user.
    std::string         itsContentServerHttpUrl;            //!< Base URL when type is "http".
    std::string         itsContentServerCorbaIor;           //!< CORBA IOR when type is "corba".
    std::string         itsPrimaryConnectionString;         //!< Primary PostgreSQL connection string when type is "postgresql".
    std::string         itsSecondaryConnectionString;       //!< Secondary PostgreSQL connection string when type is "postgresql".
    ContentServer_sptr  itsContentServer;                   //!< Active content server interface (Redis/PostgreSQL/HTTP/CORBA backend).
    bool                itsAuthenticationRequired;          //!< Whether login is required before accessing the browser UI.
    std::string         itsUsersFile;                       //!< Path to the users CSV file for authentication.
    std::string         itsGroupsFile;                      //!< Path to the groups CSV file for authorisation.
    Browser             itsBrowser;                         //!< HTML browser UI instance.
    bool                itsReadMethodsEnabled;              //!< Read methods enabled.
    bool                itsWriteMethodsEnabled;             //!< Write methods enabled.
    std::string         itsUiUrl;                           //!< URL of the new user interface (empty = disabled).
    std::string         itsUiDirectory;                     //!< Directory of the files of the new user interface.
    std::string         itsViewerUrl;                       //!< URL of the grid-gui viewer the content rows link to.
    std::map<std::string,UiFile> itsUiFiles;                //!< Files of the new user interface by name.
    std::mutex          itsUiFilesMutex;                    //!< Protects itsUiFiles.

    std::shared_ptr<Engine::Grid::Engine> itsGridEngine;    //!< Grid Engine reference used by the browser for grid-specific pages.
    ContentServer::HTTP::ServerInterface  itsMessageProcessor1;  //!< HTTP API processor backed by the direct content server.
    ContentServer::HTTP::ServerInterface  itsMessageProcessor2;  //!< HTTP API processor backed by the Grid Engine's in-memory content cache.

};  // class Plugin

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
