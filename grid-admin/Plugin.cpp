// ======================================================================
/*!
 * \brief SmartMet Admin plugin implementation
 */
// ======================================================================

#include "Plugin.h"

#include <grid-content/contentServer/corba/client/ClientImplementation.h>
#include <grid-content/contentServer/http/client/ClientImplementation.h>
#include "grid-content/contentServer/postgresql/PostgresqlImplementation.h"
#include <grid-content/contentServer/redis/RedisImplementation.h>
#include <spine/SmartMet.h>
#include <macgyver/TimeFormatter.h>
#include <macgyver/DateTime.h>
#include <boost/bind/bind.hpp>

namespace ph = boost::placeholders;

namespace SmartMet
{
namespace Plugin
{
namespace GridAdmin
{


// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(Spine::Reactor *theReactor, const char *theConfig)
    : SmartMetPlugin(), itsModuleName("GridAdmin")
{
  try
  {
    const char *configAttribute[] =
    {
        "smartmet.plugin.grid-admin.content-server.type",
        "smartmet.plugin.grid-admin.content-server.redis.address",
        "smartmet.plugin.grid-admin.content-server.redis.port",
        "smartmet.plugin.grid-admin.content-server.redis.tablePrefix",
        "smartmet.plugin.grid-admin.content-server.http.url",
        "smartmet.plugin.grid-admin.content-server.corba.ior",
        nullptr
    };

    itsReactor = theReactor;
    itsContentServerRedisAddress = "127.0.0.1";
    itsContentServerRedisPort = 6379;
    itsContentServerRedisTablePrefix = "";
    itsContentServerRedisSecondaryAddress = "127.0.0.1";
    itsContentServerRedisSecondaryPort = 0;
    itsContentServerRedisLockEnabled = false;
    itsContentServerHttpUrl = "";
    itsContentServerCorbaIor = "";
    itsAuthenticationRequired = false;
    itsUsersFile = "";
    itsGroupsFile = "";

    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
      throw Fmi::Exception(BCP, "GridContent plugin and Server API version mismatch");

    // Register the handler
    if (!theReactor->addPrivateContentHandler(this, "/grid-admin",
					      boost::bind(&Plugin::callRequestHandler,
							  this, ph::_1, ph::_2, ph::_3)))
      throw Fmi::Exception(BCP, "Failed to register GridContent request handler");

    itsConfigurationFile.readFile(theConfig);

    uint t=0;
    while (configAttribute[t] != nullptr)
    {
      if (!itsConfigurationFile.findAttribute(configAttribute[t]))
      {
        Fmi::Exception exception(BCP, "Missing configuration attribute!");
        exception.addParameter("File",theConfig);
        exception.addParameter("Attribute",configAttribute[t]);
      }
      t++;
    }

    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.authenticationRequired", itsAuthenticationRequired);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.usersFile", itsUsersFile);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.groupsFile", itsGroupsFile);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.type", itsContentServerType);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.redis.address", itsContentServerRedisAddress);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.redis.port", itsContentServerRedisPort);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.redis.tablePrefix", itsContentServerRedisTablePrefix);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.redis.secondartAddress", itsContentServerRedisSecondaryAddress);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.redis.secondaryPort", itsContentServerRedisSecondaryPort);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.redis.lockEnabled", itsContentServerRedisLockEnabled);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.http.url", itsContentServerHttpUrl);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.corba.ior", itsContentServerCorbaIor);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.postgresql.primaryConnectionString", itsPrimaryConnectionString);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.content-server.postgresql.secondaryConnectionString", itsSecondaryConnectionString);

  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

Plugin::~Plugin()
{
}




// ----------------------------------------------------------------------
/*!
 * \brief Initializator, in this case trivial
 */
// ----------------------------------------------------------------------

void Plugin::init()
{
  try
  {
    ContentServer::ServiceInterface *cServer = nullptr;
    if (itsContentServerType == "redis")
    {
      ContentServer::RedisImplementation *redis = new ContentServer::RedisImplementation();
      redis->init(itsContentServerRedisAddress.c_str(),itsContentServerRedisPort,itsContentServerRedisTablePrefix.c_str(),itsContentServerRedisSecondaryAddress.c_str(),itsContentServerRedisSecondaryPort,itsContentServerRedisLockEnabled,false);
      itsContentServer.reset(redis);
      cServer = redis;
    }
    else
    if (itsContentServerType == "postgresql")
    {
      ContentServer::PostgresqlImplementation* postgres = new ContentServer::PostgresqlImplementation();
      postgres->init(itsPrimaryConnectionString.c_str(),itsSecondaryConnectionString.c_str(),false);
      itsContentServer.reset(postgres);
      cServer = postgres;
    }
    else
    if (itsContentServerType == "corba")
    {
      ContentServer::Corba::ClientImplementation *client = new ContentServer::Corba::ClientImplementation();
      client->init(itsContentServerCorbaIor.c_str());
      itsContentServer.reset(client);
      cServer = client;
    }
    else
    if (itsContentServerType == "http")
    {
      ContentServer::HTTP::ClientImplementation *client = new ContentServer::HTTP::ClientImplementation();
      client->init(itsContentServerHttpUrl.c_str());
      itsContentServer.reset(client);
      cServer = client;
    }
    else
    {
      Fmi::Exception exception(BCP, "Unknow content server type!");
      exception.addParameter("Content server type",itsContentServerType);
    }

    auto  engine = itsReactor->getSingleton("grid", NULL);
    if (engine)
    {
      itsGridEngine = reinterpret_cast<Engine::Grid::Engine*>(engine);
    }

    itsMessageProcessor1.init(cServer);
    itsMessageProcessor2.init(itsGridEngine->getContentServer_sptr().get());

    itsBrowser.init(itsGridEngine,itsAuthenticationRequired,itsGroupsFile,itsUsersFile);
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





// ----------------------------------------------------------------------
/*!
 * \brief Shutdown the plugin
 */
// ----------------------------------------------------------------------

void Plugin::shutdown()
{
  try
  {
    std::cout << "  -- Shutdown requested (grid-content)\n";
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Plugin::apiRequest(Spine::Reactor &theReactor,const Spine::HTTP::Request &theRequest,Spine::HTTP::Response &theResponse)
{
  try
  {
    T::RequestMessage requestMessage;
    T::ResponseMessage responseMessage;

    size_t sz = theRequest.getContentLength();
    if (sz == 0)
    {
      auto mappi = theRequest.getParameterMap();
      for (auto it = mappi.begin(); it != mappi.end(); ++it)
      {
        requestMessage.addLine(it->first.c_str(),it->second.c_str());
        // std::cout << "[" << it->first << "][" << it->second << "]" << std::endl;
      }
    }
    else
    {
      std::string content = theRequest.getContent();
      const char *cont = content.c_str();
      char line[10000];
      uint c = 0;
      for (size_t t=0; t<sz; t++)
      {
        char ch = cont[t];
        if (ch == '\r'  || ch == '\n')
        {
          line[c] = '\0';
          if (c > 0)
           requestMessage.addLine(line);

          c = 0;
        }
        else
        {
          line[c] = ch;
          c++;
        }
      }

      if (c > 0)
      {
        line[c] = '\0';
        requestMessage.addLine(line);
      }
    }

    //requestMessage.print(std::cout,0,0);

    auto s = theRequest.getParameter("source");

    if (s && *s == "engine")
      itsMessageProcessor2.processRequest(requestMessage,responseMessage);  // Calling grid engine (=> Cache)
    else
      itsMessageProcessor1.processRequest(requestMessage,responseMessage);  // Calling data storage (=> Redis)
    //responseMessage.print(std::cout,0,0);

    uint lineCount = responseMessage.getLineCount();
    uint size = responseMessage.getContentSize();
    char *content = new char[size+100];
    char *p = content;

    std::unique_ptr<char[]> pContent(content); // This should automatically release the allocated space

    for (uint t=0; t<lineCount; t++)
    {
      std::string s = responseMessage.getLineByIndex(t);
      p += sprintf(p,"%s\n",s.c_str());
    }


    theResponse.setContent(std::string(content));

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}




bool Plugin::request(Spine::Reactor &theReactor,const Spine::HTTP::Request &theRequest,Spine::HTTP::Response &theResponse)
{
  try
  {

    auto method = theRequest.getParameter("method");
    if (method && *method > " ")
    {
      theResponse.setHeader("Content-Type", "text/plain; charset=UTF-8");
      return apiRequest(theReactor,theRequest,theResponse);
    }

    return itsBrowser.requestHandler(theRequest,theResponse);
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}


// ----------------------------------------------------------------------
/*!
 * \brief Main request handler
 */
// ----------------------------------------------------------------------

void Plugin::requestHandler(Spine::Reactor &theReactor,const Spine::HTTP::Request &theRequest,Spine::HTTP::Response &theResponse)
{
  try
  {
    try
    {
      // Check request method (support GET, OPTIONS)
      if (checkRequest(theRequest, theResponse, true))
      {
        return;
      }

      // We return JSON, hence we should enable CORS
      theResponse.setHeader("Access-Control-Allow-Origin", "*");

      const int expires_seconds = 1;
      Fmi::DateTime t_now = Fmi::SecondClock::universal_time();

      bool response = request(theReactor, theRequest, theResponse);

      if (response)
      {
        theResponse.setStatus(Spine::HTTP::Status::ok);
      }
      else
      {
        theResponse.setStatus(Spine::HTTP::Status::not_implemented);
      }

      // Adding response headers

      Fmi::DateTime t_expires = t_now + Fmi::Seconds(expires_seconds);
      std::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));
      std::string cachecontrol =
          "public, max-age=" + std::to_string(expires_seconds);
      std::string expiration = tformat->format(t_expires);
      std::string modification = tformat->format(t_now);

      theResponse.setHeader("Cache-Control", cachecontrol.c_str());
      theResponse.setHeader("Expires", expiration.c_str());
      theResponse.setHeader("Last-Modified", modification.c_str());
    }
    catch (...)
    {
      // Catching all exceptions

      Fmi::Exception exception(BCP, "Request processing exception!", nullptr);
      exception.addParameter("URI", theRequest.getURI());
      exception.printError();

      theResponse.setStatus(Spine::HTTP::Status::bad_request);

      // Adding the first exception information into the response header

      std::string firstMessage = exception.what();
      boost::algorithm::replace_all(firstMessage, "\n", " ");
      firstMessage = firstMessage.substr(0, 300);
      theResponse.setHeader("X-Content-Error", firstMessage.c_str());
    }
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}


// ----------------------------------------------------------------------
/*!
 * \brief Return the plugin name
 */
// ----------------------------------------------------------------------

const std::string &Plugin::getPluginName() const
{
  return itsModuleName;
}



// ----------------------------------------------------------------------
/*!
 * \brief Return the required version
 */
// ----------------------------------------------------------------------

int Plugin::getRequiredAPIVersion() const
{
  return SMARTMET_API_VERSION;
}


// ----------------------------------------------------------------------
/*!
 * \brief This is an admin plugin
 */
// ----------------------------------------------------------------------

bool Plugin::isAdminQuery(const Spine::HTTP::Request & /* theRequest */) const
{
  return true;
}

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet


/*
 * Server knows us through the 'SmartMetPlugin' virtual interface, which
 * the 'Plugin' class implements.
 */

extern "C" SmartMetPlugin *create(SmartMet::Spine::Reactor *them, const char *config)
{
  return new SmartMet::Plugin::GridAdmin::Plugin(them, config);
}


extern "C" void destroy(SmartMetPlugin *us)
{
  // This will call 'Plugin::~Plugin()' since the destructor is virtual
  delete us;
}

// ======================================================================
