#pragma once
#include <spine/SmartMetPlugin.h>
#include <spine/Reactor.h>
#include <spine/HTTP.h>
#include <grid-files/common/ConfigurationFile.h>
#include <grid-content/contentServer/http/server/ServerInterface.h>
#include "Browser.h"



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
    virtual             ~Plugin();

    const std::string&  getPluginName() const;
    int                 getRequiredAPIVersion() const;
    bool                isAdminQuery(const Spine::HTTP::Request& theRequest) const;

  protected:

    void                init();
    void                shutdown();
    void                requestHandler(Spine::Reactor& theReactor,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

  private:
    bool                request(Spine::Reactor& theReactor,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                apiRequest(Spine::Reactor &theReactor,const Spine::HTTP::Request &theRequest,Spine::HTTP::Response &theResponse);


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
    std::string         itsContentServerHttpUrl;            //!< Base URL when type is "http".
    std::string         itsContentServerCorbaIor;           //!< CORBA IOR when type is "corba".
    std::string         itsPrimaryConnectionString;         //!< Primary PostgreSQL connection string when type is "postgresql".
    std::string         itsSecondaryConnectionString;       //!< Secondary PostgreSQL connection string when type is "postgresql".
    ContentServer_sptr  itsContentServer;                   //!< Active content server interface (Redis/PostgreSQL/HTTP/CORBA backend).
    bool                itsAuthenticationRequired;          //!< Whether login is required before accessing the browser UI.
    std::string         itsUsersFile;                       //!< Path to the users CSV file for authentication.
    std::string         itsGroupsFile;                      //!< Path to the groups CSV file for authorisation.
    Browser             itsBrowser;                         //!< HTML browser UI instance.

    std::shared_ptr<Engine::Grid::Engine> itsGridEngine;    //!< Grid Engine reference used by the browser for grid-specific pages.
    ContentServer::HTTP::ServerInterface  itsMessageProcessor1;  //!< HTTP API processor backed by the direct content server.
    ContentServer::HTTP::ServerInterface  itsMessageProcessor2;  //!< HTTP API processor backed by the Grid Engine's in-memory content cache.

};  // class Plugin

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
