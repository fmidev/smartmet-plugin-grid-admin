// ======================================================================
/*!
 * \brief SmartMet Admin plugin interface
 */
// ======================================================================

#pragma once
#include <spine/SmartMetPlugin.h>
#include <spine/Reactor.h>
#include <spine/HTTP.h>
#include <grid-content/contentServer/http/server/ServerInterface.h>
#include <grid-content/contentServer/redis/RedisImplementation.h>
#include <grid-files/common/ConfigurationFile.h>


namespace SmartMet
{
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
    std::string         itsRedisAddress;
    int                 itsRedisPort;
    std::string         itsRedisTablePrefix;

    ContentServer::HTTP::ServerInterface  itsMessageProcessor;
    ContentServer::RedisImplementation    itsRedis;

};  // class Plugin

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
