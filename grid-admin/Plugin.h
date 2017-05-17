// ======================================================================
/*!
 * \brief SmartMet Admin plugin interface
 */
// ======================================================================

#pragma once
#include <spine/SmartMetPlugin.h>
#include <spine/Reactor.h>
#include <spine/HTTP.h>
#include <libconfig.h++>
#include "grid-content/contentServer/http/server/ServerInterface.h"
#include "grid-content/contentServer/redis/RedisImplementation.h"


namespace SmartMet
{
namespace Plugin
{
namespace GridAdmin
{


class Plugin : public SmartMetPlugin, private boost::noncopyable
{
  public:

    Plugin(SmartMet::Spine::Reactor* theReactor, const char* theConfig);
    virtual ~Plugin();

    const std::string& getPluginName() const;
    int getRequiredAPIVersion() const;
    bool queryIsFast(const SmartMet::Spine::HTTP::Request& theRequest) const;

  protected:

    void init();
    void shutdown();
    void requestHandler(SmartMet::Spine::Reactor& theReactor,
                      const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse);

  private:

    Plugin();
    bool request(SmartMet::Spine::Reactor& theReactor,
                      const SmartMet::Spine::HTTP::Request& theRequest,
                      SmartMet::Spine::HTTP::Response& theResponse);


    SmartMet::ContentServer::HTTP::ServerInterface itsMessageProcessor;
    SmartMet::ContentServer::RedisImplementation itsRedis;

    const std::string         itsModuleName;
    SmartMet::Spine::Reactor* itsReactor;
    libconfig::Config         itsConfig;
    std::string               itsRedisAddress;
    int                       itsRedisPort;



};  // class Plugin

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
