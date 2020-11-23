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
    virtual               ~Browser();

    void                  init(Spine::Reactor* theReactor);
    bool                  requestHandler(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

  private:

    bool                  page_start(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_engines(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_plugins(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);


    Spine::Reactor*       itsReactor;
    Engine::Grid::Engine* itsGridEngine;
};

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
