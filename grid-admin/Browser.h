#pragma once
#include <spine/SmartMetPlugin.h>
#include <spine/Reactor.h>
#include <spine/HTTP.h>
#include <grid-files/common/ConfigurationFile.h>
#include <grid-content/contentServer/http/server/ServerInterface.h>
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

    void      init(Spine::Reactor* theReactor,bool authenticationRequired,std::string& usersFile);
    bool      requestHandler(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

  private:

    void      readUsers();
    bool      page_login(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_start(std::string& sessionId,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_engines(std::string& sessionId,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool      page_plugins(std::string& sessionId,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);


    Spine::Reactor*                   itsReactor;
    Engine::Grid::Engine*             itsGridEngine;
    std::map<std::string,time_t>      itsValidSessions;
    std::string                       itsUsersFile;
    std::map<std::string,std::string> itsUsers;
    bool                              itsAuthenticationRequired;
};

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
