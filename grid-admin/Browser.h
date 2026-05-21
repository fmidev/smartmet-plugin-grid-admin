#pragma once
#include <spine/SmartMetPlugin.h>
#include <spine/Reactor.h>
#include <spine/HTTP.h>
#include <grid-files/common/ConfigurationFile.h>
#include <grid-content/contentServer/http/server/ServerInterface.h>
#include <grid-content/sessionManagement/implementation/ServiceImplementation.h>
#include <grid-content/userManagement/implementation/ServiceImplementation.h>
#include <engines/grid/Engine.h>



namespace SmartMet
{

typedef std::shared_ptr<ContentServer::ServiceInterface> ContentServer_sptr;  //!< Shared pointer to a ContentServer service interface.

typedef std::map<std::string,std::string> Filenames;  //!< Map of resource name to file path for statically served browser assets.


namespace Plugin
{
namespace GridAdmin
{

// ====================================================================================
/*! \brief HTML browser UI for the grid-admin plugin.
 *
 *  Generates HTML pages that allow operators to inspect the SmartMet Server's loaded
 *  engines, installed plugins, and software packages.  Grid-engine-specific pages
 *  (content listings, server logs, parameter mappings) are delegated to the
 *  Engine::Grid::Engine browser via Engine::Grid::Engine::browserRequest().
 *
 *  Manages in-process sessions (cookie-based) and optional SHA1 challenge-response
 *  authentication.  When authenticationRequired is true, the `grid-content-view`
 *  permission controls access to the grid-engine browser pages. */
// ====================================================================================

class Browser
{
  public:

              Browser();
    virtual   ~Browser();

    void      init(Engine::Grid::Engine* theGridEngine,bool authenticationRequired,std::string& groupsFile,std::string& usersFile);
    bool      requestHandler(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

  private:

    void      countHash(const char *key,const char *password,char *hash);
    void      readUsers();
    bool      page_login(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_start(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_engines(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_plugins(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_software(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);


    Engine::Grid::Engine*   itsGridEngine;             //!< Grid engine used to delegate grid-specific browser pages (not owned).
    T::SessionId            itsBroswerSessionId;       //!< Current browser session identifier.
    std::string             itsGroupsFile;             //!< Path to the groups CSV file for user authorisation.
    std::string             itsUsersFile;              //!< Path to the users CSV file for authentication.
    bool                    itsAuthenticationRequired; //!< Whether login is required before accessing browser pages.
};

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
