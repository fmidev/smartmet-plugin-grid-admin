// ======================================================================
/*!
 * \brief SmartMet Admin plugin interface
 */
// ======================================================================

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


    const std::string   itsModuleName;
    Spine::Reactor*     itsReactor;
    ConfigurationFile   itsConfigurationFile;
    std::string         itsContentServerType;
    std::string         itsContentServerRedisAddress;
    int                 itsContentServerRedisPort;
    std::string         itsContentServerRedisTablePrefix;
    std::string         itsContentServerRedisSecondaryAddress;
    int                 itsContentServerRedisSecondaryPort;
    bool                itsContentServerRedisLockEnabled;
    std::string         itsContentServerHttpUrl;
    std::string         itsContentServerCorbaIor;
    ContentServer_sptr  itsContentServer;
    bool                itsAuthenticationRequired;
    std::string         itsUsersFile;
    std::string         itsGroupsFile;
    Browser             itsBrowser;

    ContentServer::HTTP::ServerInterface  itsMessageProcessor;

};  // class Plugin

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
