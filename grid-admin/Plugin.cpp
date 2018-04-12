// ======================================================================
/*!
 * \brief SmartMet Admin plugin implementation
 */
// ======================================================================

#include "Plugin.h"

#include <spine/SmartMet.h>
#include <macgyver/TimeFormatter.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace SmartMet
{
namespace Plugin
{
namespace GridAdmin
{

using namespace SmartMet::Spine;


// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(SmartMet::Spine::Reactor *theReactor, const char *theConfig)
    : SmartMetPlugin(), itsModuleName("GridAdmin")
{
  try
  {
    const char *configAttribute[] =
    {
        "smartmet.plugin.grid-admin.redis.address",
        "smartmet.plugin.grid-admin.redis.port",
        "smartmet.plugin.grid-admin.redis.tablePrefix",
        NULL
    };

    itsReactor = theReactor;
    itsRedisAddress = "127.0.0.1";
    itsRedisPort = 6379;

    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
      throw SmartMet::Spine::Exception(BCP, "GridContent plugin and Server API version mismatch");

    // Register the handler
    if (!theReactor->addContentHandler(this, "/grid-admin", boost::bind(&Plugin::callRequestHandler, this, _1, _2, _3)))
      throw SmartMet::Spine::Exception(BCP, "Failed to register GridContent request handler");

    itsConfigurationFile.readFile(theConfig);

    uint t=0;
    while (configAttribute[t] != NULL)
    {
      if (!itsConfigurationFile.findAttribute(configAttribute[t]))
      {
        SmartMet::Spine::Exception exception(BCP, "Missing configuration attribute!");
        exception.addParameter("File",theConfig);
        exception.addParameter("Attribute",configAttribute[t]);
      }
      t++;
    }

    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.redis.address", itsRedisAddress);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.redis.port", itsRedisPort);
    itsConfigurationFile.getAttributeValue("smartmet.plugin.grid-admin.redis.tablePrefix", itsRedisTablePrefix);
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
    itsRedis.init(itsRedisAddress.c_str(),itsRedisPort,itsRedisTablePrefix.c_str());
    itsMessageProcessor.init(&itsRedis);

    auto engine = itsReactor->getSingleton("grid", NULL);
    if (!engine)
      throw Spine::Exception(BCP, "The 'grid-engine' unavailable!");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}



bool Plugin::request(SmartMet::Spine::Reactor &theReactor,
                            const HTTP::Request &theRequest,
                            HTTP::Response &theResponse)
{
  try
  {
    SmartMet::T::RequestMessage requestMessage;
    SmartMet::T::ResponseMessage responseMessage;

    size_t sz = theRequest.getContentLength();
    if (sz == 0)
    {
      auto mappi = theRequest.getParameterMap();
      for (auto it = mappi.begin(); it != mappi.end(); ++it)
      {
        requestMessage.addLine(it->first.c_str(),it->second.c_str());
        std::cout << "[" << it->first << "][" << it->second << "]" << std::endl;
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

    itsMessageProcessor.processRequest(requestMessage,responseMessage);
    //responseMessage.print(std::cout,0,0);

    uint lineCount = responseMessage.getLineCount();
    uint size = responseMessage.getContentSize();
    char *content = new char[size+100];
    char *p = content;

    for (uint t=0; t<lineCount; t++)
    {
      std::string s = responseMessage.getLineByIndex(t);
      p += sprintf(p,"%s\n",s.c_str());
    }


    theResponse.setContent(std::string(content));

    delete content;
    return true;
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}



// ----------------------------------------------------------------------
/*!
 * \brief Main request handler
 */
// ----------------------------------------------------------------------

void Plugin::requestHandler(SmartMet::Spine::Reactor &theReactor,
                            const HTTP::Request &theRequest,
                            HTTP::Response &theResponse)
{
  try
  {
    try
    {
      // We return JSON, hence we should enable CORS
      theResponse.setHeader("Access-Control-Allow-Origin", "*");

      const int expires_seconds = 1;
      boost::posix_time::ptime t_now = boost::posix_time::second_clock::universal_time();

      bool response = request(theReactor, theRequest, theResponse);

      if (response)
      {
        theResponse.setStatus(HTTP::Status::ok);
      }
      else
      {
        theResponse.setStatus(HTTP::Status::not_implemented);
      }

      // Adding response headers

      boost::posix_time::ptime t_expires = t_now + boost::posix_time::seconds(expires_seconds);
      boost::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));
      std::string cachecontrol =
          "public, max-age=" + boost::lexical_cast<std::string>(expires_seconds);
      std::string expiration = tformat->format(t_expires);
      std::string modification = tformat->format(t_now);

      theResponse.setHeader("Cache-Control", cachecontrol.c_str());
      theResponse.setHeader("Expires", expiration.c_str());
      theResponse.setHeader("Last-Modified", modification.c_str());
    }
    catch (...)
    {
      // Catching all exceptions

      SmartMet::Spine::Exception exception(BCP, "Request processing exception!", NULL);
      exception.addParameter("URI", theRequest.getURI());
      exception.printError();

      theResponse.setStatus(HTTP::Status::bad_request);

      // Adding the first exception information into the response header

      std::string firstMessage = exception.what();
      boost::algorithm::replace_all(firstMessage, "\n", " ");
      firstMessage = firstMessage.substr(0, 300);
      theResponse.setHeader("X-Content-Error", firstMessage.c_str());
    }
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
 * \brief Performance query implementation.
 *
 * We want admin calls to always be processed ASAP
 */
// ----------------------------------------------------------------------

bool Plugin::queryIsFast(const SmartMet::Spine::HTTP::Request & /* theRequest */) const
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
