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



namespace SmartMet
{

typedef std::shared_ptr<ContentServer::ServiceInterface> ContentServer_sptr;


namespace Plugin
{
namespace GridAdmin
{


class Plugin : public SmartMetPlugin, private boost::noncopyable
{
  public:

                        Plugin(Spine::Reactor* theReactor, const char* theConfig);
    virtual             ~Plugin();

    const std::string&  getPluginName() const;
    int                 getRequiredAPIVersion() const;
    bool                queryIsFast(const Spine::HTTP::Request& theRequest) const;

  protected:

    void                init();
    void                shutdown();
    void                requestHandler(Spine::Reactor& theReactor,const Spine::HTTP::Request& theRequest,
                            Spine::HTTP::Response& theResponse);

  private:

                        Plugin();
    bool                request(Spine::Reactor& theReactor,const Spine::HTTP::Request& theRequest,
                            Spine::HTTP::Response& theResponse);


    const std::string   itsModuleName;
    Spine::Reactor*     itsReactor;
    ConfigurationFile   itsConfigurationFile;
    std::string         itsContentServerType;
    std::string         itsContentServerRedisAddress;
    int                 itsContentServerRedisPort;
    std::string         itsContentServerRedisTablePrefix;
    std::string         itsContentServerHttpUrl;
    std::string         itsContentServerCorbaIor;
    ContentServer_sptr  itsContentServer;

    ContentServer::HTTP::ServerInterface  itsMessageProcessor;

};  // class Plugin

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
