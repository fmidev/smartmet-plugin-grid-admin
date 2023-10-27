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

typedef std::shared_ptr<ContentServer::ServiceInterface> ContentServer_sptr;

typedef std::map<std::string,std::string> Filenames;


namespace Plugin
{
namespace GridAdmin
{

class Browser
{
  public:

              Browser();
    virtual   ~Browser();

    void      init(Spine::Reactor* theReactor,bool authenticationRequired,std::string& groupsFile,std::string& usersFile);
    bool      requestHandler(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

  private:

    void      countHash(const char *key,const char *password,char *hash);
    void      readUsers();
    bool      page_login(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_start(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_engines(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_plugins(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);


    Spine::Reactor*                   itsReactor;
    Engine::Grid::Engine*             itsGridEngine;
    T::SessionId                      itsBroswerSessionId;
    std::string                       itsGroupsFile;
    std::string                       itsUsersFile;
    bool                              itsAuthenticationRequired;
};

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
