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

    void                  init(ConfigurationFile *theConfigurationFile,ContentServer_sptr theMainContentServer,Spine::Reactor* theReactor);
    bool                  requestHandler(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

  private:

    bool                  page_start(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_engines(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_configuration(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_configurationFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_producerFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_parameterMappingFiles(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_parameterMappingFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_parameterAliasFiles(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_parameterAliasFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_producerMappingFiles(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_producerMappingFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_luaFiles(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_gridEngine_luaFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_contentServer(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_contentInformation(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_producers(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_generations(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_files(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);
    bool                  page_contentList(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse);

    bool                  includeFile(std::ostringstream& output,const char *filename);


    Spine::Reactor*       itsReactor;
    Engine::Grid::Engine* itsGridEngine = nullptr;
    ContentServer_sptr    itsMainContentServer;
    ContentServer_sptr    itsCacheContentServer;
    ConfigurationFile*    itsConfigurationFile;
    uint                  itsCachedFileId;
    T::ContentInfoList    itsCachedContentInfoList;
    Filenames             itsFilenames;
};

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
