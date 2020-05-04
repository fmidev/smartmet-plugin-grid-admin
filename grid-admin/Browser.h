#pragma once
#include <spine/SmartMetPlugin.h>
#include <spine/Reactor.h>
#include <spine/HTTP.h>
#include <grid-files/common/ConfigurationFile.h>
#include <grid-content/contentServer/http/server/ServerInterface.h>



namespace SmartMet
{

typedef std::shared_ptr<ContentServer::ServiceInterface> ContentServer_sptr;


namespace Plugin
{
namespace GridAdmin
{

class Browser
{
  public:

                        Browser();
    virtual             ~Browser();

    void                init(ConfigurationFile *theConfigurationFile,ContentServer_sptr  theContentServer);
    bool                requestHandler(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

  private:

    bool                page_producers(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                page_generations(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                page_files(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                page_content(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

    ContentServer_sptr  itsContentServer;
    ConfigurationFile*  itsConfigurationFile;
    uint                itsCachedFileId;
    T::ContentInfoList  itsCachedContentInfoList;
};

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
