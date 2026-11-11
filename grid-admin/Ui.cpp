// ======================================================================
/*!
 * \brief The new user interface of grid-admin
 *
 * Everything below the user interface URL (default /grid-console):
 *
 *   /grid-console/            the page (ui/index.html)
 *   /grid-console/ui/NAME     a file of the page (JavaScript modules, CSS) from ui.directory
 *   /grid-console/api/NAME    the JSON API of the page
 *
 * The page keeps its state in its own address, the server only keeps the login in the
 * session. The earlier HTML pages and the Content Server API (method=) at /grid-admin
 * are not affected.
 *
 * Modifications (POST api/modify, api/logcontrol) need a logged-in member of the
 * grid-admin group and the corresponding flag in the browser.flags setting of the grid
 * engine, as the earlier pages did. They are made to a content source of the engine,
 * never to its cache, which follows the source.
 */
// ======================================================================

#include "Plugin.h"

#include <grid-files/common/GeneralFunctions.h>
#include <grid-files/identification/GridDef.h>
#include <grid-content/contentServer/definition/ServiceResults.h>
#include <macgyver/Hash.h>
#include <spine/HTTP.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <tuple>

namespace SmartMet
{
namespace Plugin
{
namespace GridAdmin
{

using namespace SmartMet::Spine;

namespace
{
// An error in the request: answered with 400 and the message

class BadRequest : public std::runtime_error
{
 public:
  using std::runtime_error::runtime_error;
};

// A modification the session may not make: answered with 403 and the message

class Forbidden : public std::runtime_error
{
 public:
  using std::runtime_error::runtime_error;
};

// An unknown record: answered with 404 and the message

class NotFound : public std::runtime_error
{
 public:
  using std::runtime_error::runtime_error;
};

long long optionalInteger(const HTTP::Request& theRequest, const char* name, long long defaultValue)
{
  auto value = theRequest.getParameter(name);
  if (!value || value->empty())
    return defaultValue;

  try
  {
    std::size_t pos = 0;
    long long result = std::stoll(*value, &pos);
    if (pos == value->size())
      return result;
  }
  catch (...)
  {
  }
  throw BadRequest(std::string("Parameter '") + name + "' must be an integer");
}

long long requiredInteger(const HTTP::Request& theRequest, const char* name)
{
  auto value = theRequest.getParameter(name);
  if (!value || value->empty())
    throw BadRequest(std::string("Parameter '") + name + "' is missing");
  return optionalInteger(theRequest, name, 0);
}

std::string timeString(time_t t)
{
  if (t <= 0)
    return "";
  return utcTimeFromTimeT(t);
}

// Only plain file names are served: letters, digits, '-', '_' and one '.' before a known
// extension. This keeps requests inside ui.directory.

const char* uiContentType(const std::string& name)
{
  if (name.empty() || name.size() > 64)
    return nullptr;

  std::size_t dots = 0;
  for (char ch : name)
  {
    if (ch == '.')
      dots++;
    else if (!isalnum(static_cast<unsigned char>(ch)) && ch != '-' && ch != '_')
      return nullptr;
  }
  if (dots != 1 || name[0] == '.')
    return nullptr;

  const std::string ext = name.substr(name.find('.') + 1);
  if (ext == "js")
    return "text/javascript; charset=UTF-8";
  if (ext == "css")
    return "text/css; charset=UTF-8";
  if (ext == "html")
    return "text/html; charset=UTF-8";
  if (ext == "svg")
    return "image/svg+xml";
  return nullptr;
}

int notFound(HTTP::Response& theResponse)
{
  theResponse.setHeader("Content-Type", "text/plain; charset=UTF-8");
  theResponse.setContent("Not found\n");
  return HTTP::Status::not_found;
}

const char* fileTypeName(uint fileType)
{
  switch (fileType)
  {
    case T::FileTypeValue::Grib1:
      return "GRIB1";
    case T::FileTypeValue::Grib2:
      return "GRIB2";
    case T::FileTypeValue::NetCdf3:
      return "NetCDF3";
    case T::FileTypeValue::NetCdf4:
      return "NetCDF4";
    case T::FileTypeValue::QueryData:
      return "QueryData";
    case T::FileTypeValue::GeoTiff:
      return "GeoTIFF";
    case T::FileTypeValue::Grib:
      return "GRIB";
    default:
      return "Unknown";
  }
}

const char* generationStatusName(uint status)
{
  switch (status)
  {
    case T::GenerationInfo::Status::Disabled:
      return "disabled";
    case T::GenerationInfo::Status::Ready:
      return "ready";
    case T::GenerationInfo::Status::Running:
      return "running";
    default:
      return "unknown";
  }
}

const char* serverTypeName(uint serverType)
{
  switch (serverType)
  {
    case T::FileInfo::ServerType::Filesys:
      return "filesys";
    case T::FileInfo::ServerType::S3:
      return "S3";
    case T::FileInfo::ServerType::THREDDS:
      return "THREDDS";
    case T::FileInfo::ServerType::HTTPD:
      return "HTTPD";
    default:
      return "";
  }
}

const char* protocolName(uint protocol)
{
  switch (protocol)
  {
    case T::FileInfo::Protocol::HTTP:
      return "HTTP";
    case T::FileInfo::Protocol::HTTPS:
      return "HTTPS";
    default:
      return "";
  }
}

Json::Value producerJson(const T::ProducerInfo& p)
{
  Json::Value json(Json::objectValue);
  json["id"] = Json::UInt(p.mProducerId);
  json["name"] = p.mName;
  json["title"] = p.mTitle;
  json["description"] = p.mDescription;
  json["flags"] = p.mFlags;
  json["sourceId"] = Json::UInt(p.mSourceId);
  json["status"] = p.mStatus;
  return json;
}

Json::Value generationJson(const T::GenerationInfo& g)
{
  Json::Value json(Json::objectValue);
  json["id"] = Json::UInt(g.mGenerationId);
  json["producerId"] = Json::UInt(g.mProducerId);
  json["name"] = g.mName;
  json["description"] = g.mDescription;
  json["analysisTime"] = g.mAnalysisTime;
  json["status"] = generationStatusName(g.mStatus);
  json["statusCode"] = g.mStatus;
  json["type"] = g.mGenerationType;
  json["flags"] = g.mFlags;
  json["sourceId"] = Json::UInt(g.mSourceId);
  json["start"] = timeString(g.mContentStartTime);
  json["end"] = timeString(g.mContentEndTime);
  json["modified"] = timeString(g.mModificationTime);
  json["deletion"] = timeString(g.mDeletionTime);
  return json;
}

Json::Value fileJson(const T::FileInfo& f)
{
  Json::Value json(Json::objectValue);
  json["id"] = Json::UInt(f.mFileId);
  json["producerId"] = Json::UInt(f.mProducerId);
  json["generationId"] = Json::UInt(f.mGenerationId);
  json["name"] = f.mName;
  json["type"] = fileTypeName(f.mFileType);
  json["typeCode"] = f.mFileType;
  json["server"] = f.mServer;
  json["serverType"] = serverTypeName(f.mServerType);
  json["serverTypeCode"] = f.mServerType;
  json["protocol"] = protocolName(f.mProtocol);
  json["protocolCode"] = f.mProtocol;
  json["flags"] = f.mFlags;
  json["sourceId"] = Json::UInt(f.mSourceId);
  json["modified"] = timeString(f.mModificationTime);
  json["deletion"] = timeString(f.mDeletionTime);
  return json;
}

// The names of levels and geometries, looked up once per request

class Names
{
 public:
  const std::string& level(int levelId)
  {
    auto it = itsLevels.find(levelId);
    if (it != itsLevels.end())
      return it->second;
    Identification::LevelDef def;
    std::string name;
    if (levelId >= 0 && Identification::gridDef.getFmiLevelDef(levelId, def))
      name = def.mName;
    return itsLevels.emplace(levelId, name).first->second;
  }

  const std::string& geometry(T::GeometryId geometryId)
  {
    auto it = itsGeometries.find(geometryId);
    if (it != itsGeometries.end())
      return it->second;
    std::string name;
    Identification::gridDef.getGeometryNameById(geometryId, name);
    return itsGeometries.emplace(geometryId, name).first->second;
  }

 private:
  std::map<int, std::string> itsLevels;
  std::map<T::GeometryId, std::string> itsGeometries;
};

Json::Value contentJson(const T::ContentInfo& c, Names& names)
{
  Json::Value json(Json::objectValue);
  json["message"] = Json::UInt(c.mMessageIndex);
  json["position"] = Json::UInt64(c.mFilePosition);
  json["size"] = c.mMessageSize;
  std::string parameter = c.getFmiParameterName();
  if (parameter.empty() && c.mFmiParameterId > 0)
    parameter = "FMI-" + std::to_string(c.mFmiParameterId);
  json["parameter"] = parameter;
  json["fmiParameterId"] = Json::UInt(c.mFmiParameterId);
  json["levelId"] = c.mFmiParameterLevelId;
  json["levelName"] = names.level(c.mFmiParameterLevelId);
  json["level"] = c.mParameterLevel;
  json["time"] = c.getForecastTime();
  json["forecastType"] = c.mForecastType;
  json["forecastNumber"] = c.mForecastNumber;
  json["geometry"] = c.mGeometryId;
  json["geometryName"] = names.geometry(c.mGeometryId);
  json["flags"] = c.mFlags;
  json["aggregationId"] = c.mAggregationId;
  json["aggregationPeriod"] = c.mAggregationPeriod;
  json["processingTypeId"] = c.mProcessingTypeId;
  json["processingTypeValue1"] = c.mProcessingTypeValue1;
  json["processingTypeValue2"] = c.mProcessingTypeValue2;
  json["sourceId"] = Json::UInt(c.mSourceId);
  json["modified"] = timeString(c.mModificationTime);
  json["deletion"] = timeString(c.mDeletionTime);
  return json;
}

Json::Value attributeJson(const T::AttributeNode& node)
{
  Json::Value json(Json::objectValue);
  json["name"] = node.mName;
  if (!node.mValue.empty() || node.mSubAttributes.empty())
    json["value"] = node.mValue;
  if (!node.mSubAttributes.empty())
  {
    Json::Value children(Json::arrayValue);
    for (const auto& child : node.mSubAttributes)
    {
      if (child)
        children.append(attributeJson(*child));
    }
    json["children"] = children;
  }
  return json;
}

// The description of a configured file: its path, whether it exists, size and time

Json::Value configFileJson(const std::string& kind, uint index, const std::string& path)
{
  Json::Value json(Json::objectValue);
  json["kind"] = kind;
  json["index"] = index;
  json["path"] = path;
  const time_t modificationTime = getFileModificationTime(path.c_str());
  json["exists"] = (modificationTime != 0);
  if (modificationTime != 0)
  {
    json["size"] = Json::Int64(getFileSize(path.c_str()));
    json["modified"] = timeString(modificationTime);
  }
  return json;
}

// The configuration keys of the files listed in the engine configuration, with the
// names of the kinds used in the API

const struct
{
  const char* kind;
  const char* attribute;
  const char* title;
} configFileLists[] = {
    {"parameterMapping", "smartmet.engine.grid.query-server.mappingFiles", "Parameter mapping files"},
    {"producerMapping", "smartmet.engine.grid.query-server.producerMappingFiles", "Producer mapping files"},
    {"parameterAlias", "smartmet.engine.grid.query-server.aliasFiles", "Parameter alias files"},
    {"lua", "smartmet.engine.grid.query-server.luaFiles", "Lua function files"}};

// Passwords are not shown: the value of every line mentioning a password is replaced

std::string redactPasswords(const std::string& text, uint& count)
{
  std::istringstream in(text);
  std::ostringstream out;
  std::string line;
  while (std::getline(in, line))
  {
    std::string lower = line;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return std::tolower(ch); });
    const auto pos = line.find_first_of("=:");
    const auto comment = lower.find_first_not_of(" \t");
    const bool isComment = (comment != std::string::npos && (lower[comment] == '#' || lower.compare(comment, 2, "//") == 0));
    if (!isComment && lower.find("password") != std::string::npos && pos != std::string::npos)
    {
      out << line.substr(0, pos + 1) << " \"********\"  # value hidden by grid-admin\n";
      count++;
    }
    else
      out << line << "\n";
  }
  return out.str();
}

// ### Reading the fields of a modification

class Fields
{
 public:
  explicit Fields(const Json::Value& json) : itsJson(json) {}

  bool has(const char* name) const { return itsJson.isObject() && itsJson.isMember(name); }

  std::string text(const char* name, std::size_t maxLength = 1000) const
  {
    const Json::Value& v = itsJson[name];
    if (!v.isString())
      throw BadRequest(std::string("Field '") + name + "' must be a string");
    std::string value = v.asString();
    if (value.size() > maxLength)
      throw BadRequest(std::string("Field '") + name + "' is too long");
    for (unsigned char ch : value)
    {
      if (ch < 32)
        throw BadRequest(std::string("Field '") + name + "' contains control characters");
    }
    return value;
  }

  long long integer(const char* name, long long minValue, long long maxValue) const
  {
    const Json::Value& v = itsJson[name];
    long long value = 0;
    if (v.isIntegral())
      value = v.asInt64();
    else if (v.isString())
    {
      try
      {
        std::size_t pos = 0;
        value = std::stoll(v.asString(), &pos);
        if (pos != v.asString().size())
          throw BadRequest("");
      }
      catch (...)
      {
        throw BadRequest(std::string("Field '") + name + "' must be an integer");
      }
    }
    else
      throw BadRequest(std::string("Field '") + name + "' must be an integer");
    if (value < minValue || value > maxValue)
      throw BadRequest(std::string("Field '") + name + "' must be " + std::to_string(minValue) + "..." + std::to_string(maxValue));
    return value;
  }

  double number(const char* name) const
  {
    const Json::Value& v = itsJson[name];
    if (v.isNumeric())
      return v.asDouble();
    if (v.isString())
    {
      try
      {
        std::size_t pos = 0;
        double value = std::stod(v.asString(), &pos);
        if (pos == v.asString().size() && std::isfinite(value))
          return value;
      }
      catch (...)
      {
      }
    }
    throw BadRequest(std::string("Field '") + name + "' must be a number");
  }

  // A time as YYYYMMDDTHHMMSS; an empty value is 0 (not set)
  time_t time(const char* name) const
  {
    const std::string value = text(name, 15);
    if (value.empty())
      return 0;
    if (!isTime(value))
      throw BadRequest(std::string("Field '") + name + "' must be a time YYYYMMDDTHHMMSS");
    return utcTimeToTimeT(value);
  }

  static bool isTime(const std::string& value)
  {
    if (value.size() != 15 || value[8] != 'T')
      return false;
    for (std::size_t i = 0; i < value.size(); i++)
    {
      if (i != 8 && !isdigit(static_cast<unsigned char>(value[i])))
        return false;
    }
    return true;
  }

 private:
  const Json::Value& itsJson;
};

const long long maxUInt = 0xFFFFFFFFLL;
const long long maxInt = 0x7FFFFFFFLL;

void setProducerFields(const Fields& f, T::ProducerInfo& p)
{
  if (f.has("name"))
    p.mName = f.text("name", 200);
  if (f.has("title"))
    p.mTitle = f.text("title");
  if (f.has("description"))
    p.mDescription = f.text("description");
  if (f.has("flags"))
    p.mFlags = static_cast<uint>(f.integer("flags", 0, maxUInt));
  if (f.has("sourceId"))
    p.mSourceId = static_cast<T::SourceId>(f.integer("sourceId", 0, maxUInt));
}

void setGenerationFields(const Fields& f, T::GenerationInfo& g)
{
  if (f.has("name"))
    g.mName = f.text("name", 200);
  if (f.has("description"))
    g.mDescription = f.text("description");
  if (f.has("analysisTime"))
  {
    g.mAnalysisTime = f.text("analysisTime", 15);
    if (!Fields::isTime(g.mAnalysisTime))
      throw BadRequest("Field 'analysisTime' must be a time YYYYMMDDTHHMMSS");
  }
  if (f.has("status"))
    g.mStatus = static_cast<uchar>(f.integer("status", 0, 2));
  if (f.has("flags"))
    g.mFlags = static_cast<uint>(f.integer("flags", 0, maxUInt));
  if (f.has("sourceId"))
    g.mSourceId = static_cast<T::SourceId>(f.integer("sourceId", 0, maxUInt));
  if (f.has("deletion"))
    g.mDeletionTime = f.time("deletion");
}

void setFileFields(const Fields& f, T::FileInfo& file)
{
  if (f.has("name"))
    file.mName = f.text("name", 4000);
  if (f.has("server"))
    file.mServer = f.text("server", 1000);
  if (f.has("typeCode"))
    file.mFileType = static_cast<uchar>(f.integer("typeCode", 0, 255));
  if (f.has("protocolCode"))
    file.mProtocol = static_cast<uint>(f.integer("protocolCode", 0, 2));
  if (f.has("serverTypeCode"))
    file.mServerType = static_cast<uint>(f.integer("serverTypeCode", 0, 4));
  if (f.has("flags"))
    file.mFlags = static_cast<uint>(f.integer("flags", 0, maxUInt));
  if (f.has("sourceId"))
    file.mSourceId = static_cast<T::SourceId>(f.integer("sourceId", 0, maxUInt));
  if (f.has("deletion"))
    file.mDeletionTime = f.time("deletion");
}

void setContentFields(const Fields& f, T::ContentInfo& c)
{
  if (f.has("position"))
    c.mFilePosition = static_cast<T::FilePosition>(f.integer("position", 0, 0x7FFFFFFFFFFFLL));
  if (f.has("size"))
    c.mMessageSize = static_cast<uint>(f.integer("size", 0, maxUInt));
  if (f.has("parameter"))
    c.setFmiParameterName(f.text("parameter", 100).c_str());
  if (f.has("fmiParameterId"))
    c.mFmiParameterId = static_cast<T::FmiParamId>(f.integer("fmiParameterId", 0, maxUInt));
  if (f.has("levelId"))
    c.mFmiParameterLevelId = static_cast<T::ParamLevelId>(f.integer("levelId", 0, 255));
  if (f.has("level"))
    c.mParameterLevel = static_cast<T::ParamLevel>(f.integer("level", -maxInt - 1, maxInt));
  if (f.has("time"))
  {
    const std::string value = f.text("time", 15);
    if (!Fields::isTime(value))
      throw BadRequest("Field 'time' must be a time YYYYMMDDTHHMMSS");
    c.setForecastTime(value);
  }
  if (f.has("forecastType"))
    c.mForecastType = static_cast<T::ForecastType>(f.integer("forecastType", -32768, 32767));
  if (f.has("forecastNumber"))
    c.mForecastNumber = static_cast<T::ForecastNumber>(f.integer("forecastNumber", -32768, 32767));
  if (f.has("geometry"))
    c.mGeometryId = static_cast<T::GeometryId>(f.integer("geometry", 0, maxInt));
  if (f.has("aggregationId"))
    c.mAggregationId = static_cast<int>(f.integer("aggregationId", -maxInt - 1, maxInt));
  if (f.has("aggregationPeriod"))
    c.mAggregationPeriod = static_cast<int>(f.integer("aggregationPeriod", -maxInt - 1, maxInt));
  if (f.has("processingTypeId"))
    c.mProcessingTypeId = static_cast<int>(f.integer("processingTypeId", -maxInt - 1, maxInt));
  if (f.has("processingTypeValue1"))
    c.mProcessingTypeValue1 = static_cast<float>(f.number("processingTypeValue1"));
  if (f.has("processingTypeValue2"))
    c.mProcessingTypeValue2 = static_cast<float>(f.number("processingTypeValue2"));
  if (f.has("flags"))
    c.mFlags = static_cast<ushort>(f.integer("flags", 0, 65535));
  if (f.has("sourceId"))
    c.mSourceId = static_cast<T::SourceId>(f.integer("sourceId", 0, maxUInt));
  if (f.has("deletion"))
    c.mDeletionTime = f.time("deletion");
}

// A failed call of the content server: answered with 400 and its result text

class Rejected : public std::runtime_error
{
 public:
  using std::runtime_error::runtime_error;
};

void check(int result, const std::string& what)
{
  if (result != 0)
    throw Rejected(what + " failed: " + ContentServer::getResultString(result));
}

}  // namespace

/*! \brief Serves a request below the user interface URL. The path is the part of the
    resource after the URL, for example "/api/producers". */

int Plugin::ui_request(const HTTP::Request& theRequest, HTTP::Response& theResponse, const std::string& path)
{
  try
  {
    // Relative references in the page need the trailing slash
    if (path.empty())
    {
      theResponse.setHeader("Location", itsUiUrl + "/");
      theResponse.setContent("");
      return HTTP::Status::moved_permanently;
    }

    if (path == "/")
      return ui_file("index.html", theRequest, theResponse);

    if (path.compare(0, 4, "/ui/") == 0)
      return ui_file(path.substr(4), theRequest, theResponse);

    if (path.compare(0, 5, "/api/") == 0)
      return ui_api(path.substr(5), theRequest, theResponse);

    return notFound(theResponse);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!").addParameter("Path", path);
  }
}

/*! \brief Serves a file of the page from ui.directory. The files are kept in memory and
    read again when they change, so that the page can be edited without a restart. */

int Plugin::ui_file(const std::string& name, const HTTP::Request& theRequest, HTTP::Response& theResponse)
{
  try
  {
    const char* contentType = uiContentType(name);
    if (contentType == nullptr)
      return notFound(theResponse);

    const std::string filename = itsUiDirectory + "/" + name;
    const time_t modificationTime = getFileModificationTime(filename.c_str());
    if (modificationTime == 0)
      return notFound(theResponse);

    std::string content;
    std::string etag;
    {
      std::lock_guard<std::mutex> lock(itsUiFilesMutex);
      auto& file = itsUiFiles[name];
      if (file.modificationTime != modificationTime)
      {
        std::ifstream in(filename, std::ios::binary);
        if (!in)
          return notFound(theResponse);
        std::ostringstream buffer;
        buffer << in.rdbuf();
        file.content = buffer.str();
        file.modificationTime = modificationTime;
        file.etag = "\"grid-console-" + std::to_string(Fmi::hash_value(file.content)) + "\"";
      }
      content = file.content;
      etag = file.etag;
    }

    theResponse.setHeader("ETag", etag);
    // Revalidated on every use: an edited file is taken into use at once
    theResponse.setHeader("Cache-Control", "no-cache");
    theResponse.setHeader("Content-Type", contentType);

    if (auto status = HTTP::conditionalResponseStatus(theRequest, etag))
      return *status;

    theResponse.setContent(content);
    return HTTP::Status::ok;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!").addParameter("File", name);
  }
}

/*! \brief Serves /grid-console/api/NAME.

    The session is opened for every call. "session", "login" and "logout" are available
    to everybody, the rest only to sessions allowed to view the grid engine information.
    Login and logout must be POST requests with the X-Grid-Console header: a cross-site form
    cannot send such a request, and a cross-site script cannot without a CORS preflight,
    which this server does not answer. */

int Plugin::ui_api(const std::string& name, const HTTP::Request& theRequest, HTTP::Response& theResponse)
{
  try
  {
    using ApiHandler = Json::Value (Plugin::*)(const HTTP::Request&);
    static const std::map<std::string, ApiHandler> handlers = {
        {"producers", &Plugin::api_producers},
        {"generations", &Plugin::api_generations},
        {"files", &Plugin::api_files},
        {"contents", &Plugin::api_contents},
        {"configfile", &Plugin::api_configfile},
        {"log", &Plugin::api_log}};

    Json::Value result(Json::objectValue);
    int status = HTTP::Status::ok;

    try
    {
      SessionManagement::SessionInfo session;
      itsBrowser.openSession(theRequest, theResponse, session);

      const bool post = (theRequest.getMethod() == HTTP::RequestMethod::POST);
      const bool modification = (name == "modify" || name == "logcontrol");

      if (modification && (!post || !theRequest.getHeader("X-Grid-Console")))
      {
        status = (post ? HTTP::Status::forbidden : HTTP::Status::bad_request);
        result["error"] = (post ? "The X-Grid-Console header is missing" : "Use POST");
      }
      else if (name == "session")
        result = api_session(session);
      else if (name == "login" || name == "logout")
      {
        if (!post)
        {
          if (name != "login")
            throw BadRequest("Use POST");
          // The challenge key for the answer
          result["key"] = itsBrowser.newLoginKey(session);
        }
        else
        {
          if (!theRequest.getHeader("X-Grid-Console"))
          {
            status = HTTP::Status::forbidden;
            result["error"] = "The X-Grid-Console header is missing";
          }
          else
          {
            if (name == "login")
            {
              if (!itsBrowser.login(theRequest, session))
              {
                status = HTTP::Status::forbidden;
                result["error"] = "Login failed";
              }
            }
            else
              itsBrowser.logout(theRequest, theResponse, session);
            if (status == HTTP::Status::ok)
              result = api_session(session);
          }
        }
      }
      else if (!itsBrowser.canView(session))
      {
        status = (session.mUserInfo.getUserId() == 0 ? HTTP::Status::unauthorized : HTTP::Status::forbidden);
        result["error"] = (status == HTTP::Status::unauthorized ? "Login required" : "No permission to view the grid engine information");
      }
      else if (!itsGridEngine->isEnabled() || !itsGridEngine->isBrowserEnabled())
      {
        status = HTTP::Status::service_unavailable;
        result["error"] = (itsGridEngine->isEnabled() ? "The admin pages are disabled in the grid engine configuration (browser.enabled)"
                                                       : "The grid engine is disabled");
      }
      else if (name == "modify")
        result = api_modify(theRequest, session);
      else if (name == "logcontrol")
        result = api_logcontrol(theRequest, session);
      else if (name == "software")
        result = api_software();
      else if (name == "overview")
        result = api_overview();
      else if (name == "config")
        result = api_config();
      else if (name == "logs")
        result = api_logs();
      else
      {
        auto it = handlers.find(name);
        if (it == handlers.end())
        {
          status = HTTP::Status::not_found;
          result["error"] = "Unknown API call: " + name;
        }
        else
          result = (this->*(it->second))(theRequest);
      }
    }
    catch (const BadRequest& e)
    {
      status = HTTP::Status::bad_request;
      result = Json::Value(Json::objectValue);
      result["error"] = e.what();
    }
    catch (const NotFound& e)
    {
      status = HTTP::Status::not_found;
      result = Json::Value(Json::objectValue);
      result["error"] = e.what();
    }
    catch (const Forbidden& e)
    {
      status = HTTP::Status::forbidden;
      result = Json::Value(Json::objectValue);
      result["error"] = e.what();
    }
    catch (const Rejected& e)
    {
      // The content server refused: 400 with its reason (spine has no 409 Conflict)
      status = HTTP::Status::bad_request;
      result = Json::Value(Json::objectValue);
      result["error"] = e.what();
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder["emitUTF8"] = true;
    // Administrative information: never stored in caches
    theResponse.setHeader("Cache-Control", "no-store");
    theResponse.setHeader("Content-Type", "application/json; charset=UTF-8");
    theResponse.setContent(Json::writeString(builder, result) + "\n");
    return status;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!").addParameter("API", name);
  }
}

/*! \brief The login state of the session and the addresses the page links to. */

Json::Value Plugin::api_session(SessionManagement::SessionInfo& session)
{
  try
  {
    Json::Value result(Json::objectValue);
    const bool required = itsBrowser.authenticationRequired();
    result["authenticationRequired"] = required;
    result["authenticated"] = (!required || session.mUserInfo.getUserId() != 0);
    result["canView"] = itsBrowser.canView(session);
    if (session.mUserInfo.getUserId() != 0)
    {
      result["user"] = session.mUserInfo.getUsername();
      result["fullName"] = session.mUserInfo.getFullName();
    }
    result["legacyUrl"] = "/grid-admin";
    result["viewerUrl"] = itsViewerUrl;
    // What the session may modify. The server checks these again for every modification.
    result["canModify"] = canModify(session);
    result["canControlLogs"] = canControlLogs(session);
    // Why a permission is missing, so that the page can say it instead of silently
    // hiding the controls
    if (!result["canModify"].asBool())
      result["modifyNote"] = permissionNote(
          session, Engine::Grid::Browser::Flags::contentModificationEnabled,
          "contentModificationEnabled (bit 0)", "modify the content information");
    if (!result["canControlLogs"].asBool())
      result["logControlNote"] = permissionNote(
          session, Engine::Grid::Browser::Flags::logModificationEnabled,
          "logModificationEnabled (bit 1)", "control the logs");
    return result;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The content sources of the engine. Connection strings and passwords are not
    shown. */

Json::Value Plugin::sourcesJson()
{
  try
  {
    Json::Value result(Json::arrayValue);

    Json::Value cache(Json::objectValue);
    cache["id"] = "cache";
    cache["type"] = "cache";
    cache["name"] = "Grid engine cache";
    cache["enabled"] = true;
    result.append(cache);

    uint idx = 0;
    for (const auto& source : itsGridEngine->getContentSources())
    {
      Json::Value json(Json::objectValue);
      json["id"] = std::to_string(idx);
      json["type"] = source.mType;
      json["enabled"] = source.mEnabled;
      std::string name = source.mType + " " + std::to_string(idx);
      if (source.mType == "redis")
        name = "Redis " + source.mRedisAddress + ":" + std::to_string(source.mRedisPort) + " " + source.mRedisTablePrefix;
      else if (source.mType == "memory")
        name = "Memory " + source.mMemoryContentDir;
      json["name"] = name;
      result.append(json);
      idx++;
    }
    return result;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The content server of the source parameter: "cache" (the default) is the
    content cache of the grid engine, a number is a content source of the engine. */

ContentServer_sptr Plugin::requestedContentServer(const HTTP::Request& theRequest)
{
  try
  {
    auto source = theRequest.getParameter("source");
    if (!source || source->empty() || *source == "cache")
      return itsGridEngine->getContentServer_sptr();

    const long long idx = optionalInteger(theRequest, "source", -1);
    if (idx < 0 || idx >= static_cast<long long>(itsGridEngine->getContentSources().size()))
      throw BadRequest("Unknown source: " + *source);

    auto server = itsGridEngine->getContentSourceServer_sptr(static_cast<uint>(idx));
    if (!server)
      throw BadRequest("The source is not available: " + *source);
    return server;
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The state of the grid engine as a tree, and its content sources. */

Json::Value Plugin::api_overview()
{
  try
  {
    std::shared_ptr<T::AttributeNode> state(new T::AttributeNode("Grid engine"));
    itsGridEngine->getStateAttributes(state);

    Json::Value result(Json::objectValue);
    result["state"] = attributeJson(*state);
    result["sources"] = sourcesJson();
    result["time"] = timeString(time(nullptr));
    return result;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The producers of a source with the number of their generations and the newest
    analysis time. */

Json::Value Plugin::api_producers(const HTTP::Request& theRequest)
{
  try
  {
    auto contentServer = requestedContentServer(theRequest);

    T::ProducerInfoList producerInfoList;
    contentServer->getProducerInfoList(0, producerInfoList);
    producerInfoList.sortByName();

    T::GenerationInfoList generationInfoList;
    contentServer->getGenerationInfoList(0, generationInfoList);

    struct State
    {
      uint count = 0;
      std::string newest;
    };
    std::map<T::ProducerId, State> states;
    const uint glen = generationInfoList.getLength();
    for (uint t = 0; t < glen; t++)
    {
      const T::GenerationInfo* g = generationInfoList.getGenerationInfoByIndex(t);
      if (g == nullptr)
        continue;
      auto& state = states[g->mProducerId];
      state.count++;
      if (state.newest < g->mAnalysisTime)
        state.newest = g->mAnalysisTime;
    }

    Json::Value producers(Json::arrayValue);
    const uint len = producerInfoList.getLength();
    for (uint t = 0; t < len; t++)
    {
      const T::ProducerInfo* p = producerInfoList.getProducerInfoByIndex(t);
      if (p == nullptr)
        continue;
      Json::Value producer = producerJson(*p);
      const auto& state = states[p->mProducerId];
      producer["generations"] = state.count;
      producer["newest"] = state.newest;
      producers.append(producer);
    }

    Json::Value result(Json::objectValue);
    result["producers"] = producers;
    return result;
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The generations of a producer, newest first. In the cache the number of
    files of each generation is included: counting them in a database could be slow. */

Json::Value Plugin::api_generations(const HTTP::Request& theRequest)
{
  try
  {
    const auto producerId = static_cast<T::ProducerId>(requiredInteger(theRequest, "producer"));
    auto contentServer = requestedContentServer(theRequest);
    auto source = theRequest.getParameter("source");
    const bool cache = (!source || source->empty() || *source == "cache");

    T::ProducerInfo producerInfo;
    if (contentServer->getProducerInfoById(0, producerId, producerInfo) != 0)
      throw NotFound("Unknown producer: " + std::to_string(producerId));

    T::GenerationInfoList all;
    contentServer->getGenerationInfoListByProducerId(0, producerId, all);

    std::vector<const T::GenerationInfo*> generations;
    const uint len = all.getLength();
    for (uint t = 0; t < len; t++)
    {
      const T::GenerationInfo* g = all.getGenerationInfoByIndex(t);
      if (g != nullptr && g->mProducerId == producerId)
        generations.push_back(g);
    }
    std::sort(generations.begin(),
              generations.end(),
              [](const T::GenerationInfo* a, const T::GenerationInfo* b)
              { return std::tie(a->mAnalysisTime, a->mGenerationId) > std::tie(b->mAnalysisTime, b->mGenerationId); });

    Json::Value list(Json::arrayValue);
    for (const auto* g : generations)
    {
      Json::Value generation = generationJson(*g);
      if (cache)
      {
        uint count = 0;
        if (contentServer->getFileInfoCountByGenerationId(0, g->mGenerationId, count) == 0)
          generation["files"] = count;
      }
      list.append(generation);
    }

    Json::Value result(Json::objectValue);
    result["producer"] = producerJson(producerInfo);
    result["generations"] = list;
    return result;
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (const NotFound&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief One page of the files of a generation in the order of their ids, starting
    from the id given by "after" (inclusive). "next" is the start of the next page, or
    null when this is the last one. */

Json::Value Plugin::api_files(const HTTP::Request& theRequest)
{
  try
  {
    const auto generationId = static_cast<T::GenerationId>(requiredInteger(theRequest, "generation"));
    const long long after = optionalInteger(theRequest, "after", 0);
    const long long limit = optionalInteger(theRequest, "limit", 100);
    if (after < 0)
      throw BadRequest("Parameter 'after' must not be negative");
    if (limit < 1 || limit > 1000)
      throw BadRequest("Parameter 'limit' must be 1...1000");

    auto contentServer = requestedContentServer(theRequest);
    auto source = theRequest.getParameter("source");
    const bool cache = (!source || source->empty() || *source == "cache");

    T::GenerationInfo generationInfo;
    if (contentServer->getGenerationInfoById(0, generationId, generationInfo) != 0)
      throw NotFound("Unknown generation: " + std::to_string(generationId));

    T::ProducerInfo producerInfo;
    contentServer->getProducerInfoById(0, generationInfo.mProducerId, producerInfo);

    // One more than asked tells whether there is a next page
    T::FileInfoList fileInfoList;
    contentServer->getFileInfoListByGenerationId(0, generationId, static_cast<T::FileId>(after), static_cast<int>(limit + 1), fileInfoList);

    std::vector<const T::FileInfo*> files;
    const uint len = fileInfoList.getLength();
    for (uint t = 0; t < len; t++)
    {
      const T::FileInfo* f = fileInfoList.getFileInfoByIndex(t);
      if (f != nullptr && f->mGenerationId == generationId && f->mFileId >= static_cast<T::FileId>(after))
        files.push_back(f);
    }
    std::sort(files.begin(), files.end(), [](const T::FileInfo* a, const T::FileInfo* b) { return a->mFileId < b->mFileId; });

    Json::Value list(Json::arrayValue);
    Json::Value next;  // null
    for (std::size_t i = 0; i < files.size(); i++)
    {
      if (static_cast<long long>(i) == limit)
      {
        next = Json::UInt(files[i]->mFileId);
        break;
      }
      list.append(fileJson(*files[i]));
    }

    Json::Value result(Json::objectValue);
    result["producer"] = producerJson(producerInfo);
    result["generation"] = generationJson(generationInfo);
    if (cache)
    {
      uint count = 0;
      if (contentServer->getFileInfoCountByGenerationId(0, generationId, count) == 0)
        result["count"] = count;
    }
    result["after"] = Json::Int64(after);
    result["files"] = list;
    result["next"] = next;
    return result;
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (const NotFound&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief A file with its producer and generation, and a page of its messages. */

Json::Value Plugin::api_contents(const HTTP::Request& theRequest)
{
  try
  {
    const auto fileId = static_cast<T::FileId>(requiredInteger(theRequest, "file"));
    const long long offset = optionalInteger(theRequest, "offset", 0);
    const long long limit = optionalInteger(theRequest, "limit", 500);
    if (offset < 0)
      throw BadRequest("Parameter 'offset' must not be negative");
    if (limit < 1 || limit > 5000)
      throw BadRequest("Parameter 'limit' must be 1...5000");

    auto contentServer = requestedContentServer(theRequest);

    T::FileInfo fileInfo;
    if (contentServer->getFileInfoById(0, fileId, fileInfo) != 0)
      throw NotFound("Unknown file: " + std::to_string(fileId));

    T::GenerationInfo generationInfo;
    contentServer->getGenerationInfoById(0, fileInfo.mGenerationId, generationInfo);
    T::ProducerInfo producerInfo;
    contentServer->getProducerInfoById(0, fileInfo.mProducerId, producerInfo);

    T::ContentInfoList contentInfoList;
    contentServer->getContentListByFileId(0, fileId, contentInfoList);

    std::vector<const T::ContentInfo*> contents;
    const uint len = contentInfoList.getLength();
    for (uint t = 0; t < len; t++)
    {
      const T::ContentInfo* c = contentInfoList.getContentInfoByIndex(t);
      if (c != nullptr)
        contents.push_back(c);
    }
    std::sort(contents.begin(), contents.end(), [](const T::ContentInfo* a, const T::ContentInfo* b) { return a->mMessageIndex < b->mMessageIndex; });

    Names names;
    Json::Value list(Json::arrayValue);
    for (std::size_t i = static_cast<std::size_t>(offset); i < contents.size() && i < static_cast<std::size_t>(offset + limit); i++)
      list.append(contentJson(*contents[i], names));

    Json::Value result(Json::objectValue);
    result["producer"] = producerJson(producerInfo);
    result["generation"] = generationJson(generationInfo);
    result["file"] = fileJson(fileInfo);
    result["count"] = Json::UInt(contents.size());
    result["offset"] = Json::Int64(offset);
    result["contents"] = list;
    return result;
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (const NotFound&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The configuration files of the grid engine: the engine configuration, the
    producer search file and the files listed in the engine configuration. */

Json::Value Plugin::api_config()
{
  try
  {
    Json::Value groups(Json::arrayValue);

    Json::Value main(Json::objectValue);
    main["title"] = "Grid engine";
    main["files"] = Json::Value(Json::arrayValue);
    main["files"].append(configFileJson("engine", 0, itsGridEngine->getConfigurationFileName()));
    main["files"].append(configFileJson("producers", 0, itsGridEngine->getProducerFileName()));
    groups.append(main);

    ConfigurationFile configurationFile;
    configurationFile.readFile(itsGridEngine->getConfigurationFileName().c_str());

    for (const auto& list : configFileLists)
    {
      std::vector<std::string> files;
      configurationFile.getAttributeValue(list.attribute, files);

      Json::Value group(Json::objectValue);
      group["title"] = list.title;
      group["files"] = Json::Value(Json::arrayValue);
      for (uint i = 0; i < files.size(); i++)
        group["files"].append(configFileJson(list.kind, i, files[i]));
      groups.append(group);
    }

    Json::Value result(Json::objectValue);
    result["groups"] = groups;
    return result;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The text of one configured file. Only files named in the configuration can be
    read: the file is chosen by its kind and index, never by a path in the request. */

Json::Value Plugin::api_configfile(const HTTP::Request& theRequest)
{
  try
  {
    auto kind = theRequest.getParameter("kind");
    if (!kind || kind->empty())
      throw BadRequest("Parameter 'kind' is missing");
    const long long index = optionalInteger(theRequest, "index", 0);

    std::string path;
    if (*kind == "engine" && index == 0)
      path = itsGridEngine->getConfigurationFileName();
    else if (*kind == "producers" && index == 0)
      path = itsGridEngine->getProducerFileName();
    else
    {
      for (const auto& list : configFileLists)
      {
        if (*kind != list.kind)
          continue;
        ConfigurationFile configurationFile;
        configurationFile.readFile(itsGridEngine->getConfigurationFileName().c_str());
        std::vector<std::string> files;
        configurationFile.getAttributeValue(list.attribute, files);
        if (index >= 0 && index < static_cast<long long>(files.size()))
          path = files[index];
      }
    }

    if (path.empty())
      throw NotFound("Unknown file: " + *kind + " " + std::to_string(index));

    Json::Value result = configFileJson(*kind, static_cast<uint>(index), path);

    std::ifstream in(path, std::ios::binary);
    if (!in)
      throw NotFound("Cannot read the file: " + path);

    // Large files are cut: the page is for looking, not for downloading
    const std::size_t maxSize = 4 * 1024 * 1024;
    std::string text(maxSize, '\0');
    in.read(text.data(), static_cast<std::streamsize>(maxSize));
    text.resize(static_cast<std::size_t>(in.gcount()));
    result["truncated"] = (in.peek() != EOF);

    uint redacted = 0;
    result["text"] = redactPasswords(text, redacted);
    result["redacted"] = redacted;
    return result;
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (const NotFound&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

namespace
{
// Only regular files are read. A log may be configured as /dev/stdout, which under systemd
// is a pipe or a socket: reading it could block or take the output of the server.

bool isRegularFile(const std::string& path)
{
  std::error_code ec;
  return !path.empty() && std::filesystem::is_regular_file(path, ec);
}

// The logs of the three servers of the grid engine

Log* serverLog(Engine::Grid::Engine& engine, const std::string& server, const std::string& type)
{
  const bool processing = (type == "processing");
  if (!processing && type != "debug")
    return nullptr;

  if (server == "content")
  {
    auto s = engine.getContentServer_sptr();
    return s ? (processing ? s->getProcessingLog() : s->getDebugLog()) : nullptr;
  }
  if (server == "data")
  {
    auto s = engine.getDataServer_sptr();
    return s ? (processing ? s->getProcessingLog() : s->getDebugLog()) : nullptr;
  }
  if (server == "query")
  {
    auto s = engine.getQueryServer_sptr();
    return s ? (processing ? s->getProcessingLog() : s->getDebugLog()) : nullptr;
  }
  return nullptr;
}

Json::Value logJson(Log* log, const std::string& server, const std::string& type)
{
  Json::Value json(Json::objectValue);
  json["server"] = server;
  json["type"] = type;
  json["available"] = (log != nullptr);
  if (log != nullptr)
  {
    const std::string filename = log->getFileName();
    json["file"] = filename;
    json["enabled"] = log->isEnabled();
    json["regularFile"] = isRegularFile(filename);
    std::error_code ec;
    json["exists"] = !filename.empty() && std::filesystem::exists(filename, ec);
    if (isRegularFile(filename))
    {
      json["size"] = Json::Int64(getFileSize(filename.c_str()));
      json["modified"] = timeString(getFileModificationTime(filename.c_str()));
    }
  }
  return json;
}

const char* const logServers[] = {"content", "data", "query"};
const char* const logTypes[] = {"processing", "debug"};

}  // namespace

/*! \brief The processing and debug logs of the content, data and query servers. */

Json::Value Plugin::api_logs()
{
  try
  {
    Json::Value list(Json::arrayValue);
    for (const char* server : logServers)
      for (const char* type : logTypes)
        list.append(logJson(serverLog(*itsGridEngine, server, type), server, type));

    Json::Value result(Json::objectValue);
    result["logs"] = list;
    return result;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The last lines of a log. */

Json::Value Plugin::api_log(const HTTP::Request& theRequest)
{
  try
  {
    auto server = theRequest.getParameter("server");
    auto type = theRequest.getParameter("type");
    if (!server || !type)
      throw BadRequest("Parameters 'server' and 'type' are required");
    const long long lines = optionalInteger(theRequest, "lines", 200);
    if (lines < 1 || lines > 10000)
      throw BadRequest("Parameter 'lines' must be 1...10000");

    bool known = false;
    for (const char* s : logServers)
      known = known || (*server == s);
    if (!known || (*type != "processing" && *type != "debug"))
      throw NotFound("Unknown log: " + *server + " " + *type);

    Log* log = serverLog(*itsGridEngine, *server, *type);
    Json::Value result = logJson(log, *server, *type);
    Json::Value text(Json::arrayValue);
    if (log != nullptr)
    {
      const std::string filename = log->getFileName();
      if (isRegularFile(filename))
      {
        std::vector<std::string> eofLines;
        try
        {
          readEofLines(filename.c_str(), static_cast<uint>(lines), eofLines);
        }
        catch (...)
        {
          // A log being truncated at the same time: shown empty
        }
        for (const auto& line : eofLines)
          text.append(line);
      }
    }
    result["lines"] = text;
    return result;
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (const NotFound&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief Whether the session may modify the content information: a logged-in member
    of the grid-admin group, and content modification enabled in browser.flags. */

bool Plugin::canModify(SessionManagement::SessionInfo& session)
{
  return (itsGridEngine->getBrowserFlags() & Engine::Grid::Browser::Flags::contentModificationEnabled) != 0 &&
         itsBrowser.isAdministrator(session);
}

/*! \brief The first missing condition of a modification permission as a sentence. */

std::string Plugin::permissionNote(SessionManagement::SessionInfo& session,
                                   std::uint64_t flag,
                                   const std::string& flagName,
                                   const std::string& action)
{
  if (!itsBrowser.authenticationRequired())
    return "Read-only: to " + action +
           ", set authenticationRequired = true in the grid-admin configuration and log in as a "
           "member of the grid-admin group.";
  if (session.mUserInfo.getUserId() == 0)
    return "Read-only: log in as a member of the grid-admin group to " + action + ".";
  if (!itsBrowser.isAdministrator(session))
    return std::string("Read-only: user ") + session.mUserInfo.getUsername() +
           " is not a member of the grid-admin group needed to " + action + ".";
  if ((itsGridEngine->getBrowserFlags() & flag) == 0)
    return "Read-only: " + flagName +
           " is not set in browser.flags of the grid engine configuration.";
  return "";
}

/*! \brief Whether the session may enable, disable and clear the logs. */

bool Plugin::canControlLogs(SessionManagement::SessionInfo& session)
{
  return (itsGridEngine->getBrowserFlags() & Engine::Grid::Browser::Flags::logModificationEnabled) != 0 &&
         itsBrowser.isAdministrator(session);
}

namespace
{
// The JSON body of a modification

Json::Value requestJson(const HTTP::Request& theRequest)
{
  const std::string content = theRequest.getContent();
  if (content.size() > 65536)
    throw BadRequest("The request is too large");

  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value json;
  std::string errors;
  if (!reader->parse(content.data(), content.data() + content.size(), &json, &errors) || !json.isObject())
    throw BadRequest("The request must be a JSON object");
  return json;
}

std::string member(const Json::Value& json, const char* name)
{
  const Json::Value& v = json[name];
  if (v.isString())
    return v.asString();
  if (v.isIntegral())
    return std::to_string(v.asInt64());
  return "";
}

unsigned long long requiredId(const Json::Value& json, const char* name)
{
  const std::string value = member(json, name);
  if (value.empty() || value.size() > 20 || value.find_first_not_of("0123456789") != std::string::npos)
    throw BadRequest(std::string("'") + name + "' must be given as a non-negative integer");
  return std::stoull(value);
}
}  // namespace

/*! \brief Adds, updates or deletes a producer, a generation, a file or a message.

    The request is a JSON object:

      {"source": "0", "type": "producer|generation|file|content",
       "action": "add|update|delete", ids..., "fields": {...}}

    The ids are producer, generation, file and message as needed: the record itself for
    update and delete, its parent for add (a message is added with its file and message
    index). An update reads the record first and changes only the given fields. The
    modification is made to the content source; the cache of the engine follows it. */

Json::Value Plugin::api_modify(const HTTP::Request& theRequest, SessionManagement::SessionInfo& session)
{
  try
  {
    if (!canModify(session))
      throw Forbidden("Modifying the content information needs a member of the grid-admin group and contentModificationEnabled in browser.flags");

    const Json::Value json = requestJson(theRequest);

    const std::string source = member(json, "source");
    if (source.empty() || source == "cache" || source.find_first_not_of("0123456789") != std::string::npos ||
        std::stoull(source) >= itsGridEngine->getContentSources().size())
      throw BadRequest("'source' must be the index of a content source: the cache cannot be modified");
    auto contentServer = itsGridEngine->getContentSourceServer_sptr(static_cast<uint>(std::stoull(source)));
    if (!contentServer)
      throw BadRequest("The source is not available: " + source);

    const std::string type = member(json, "type");
    const std::string action = member(json, "action");
    if (action != "add" && action != "update" && action != "delete")
      throw BadRequest("'action' must be add, update or delete");

    const Json::Value& fieldsJson = json["fields"];
    if (action != "delete" && !fieldsJson.isObject())
      throw BadRequest("'fields' must be an object");
    const Fields fields(fieldsJson);

    Json::Value result(Json::objectValue);
    result["type"] = type;
    result["action"] = action;

    if (type == "producer")
    {
      if (action == "add")
      {
        T::ProducerInfo p;
        setProducerFields(fields, p);
        if (p.mName.empty())
          throw BadRequest("A producer needs a name");
        check(contentServer->addProducerInfo(0, p), "Adding the producer");
        result["id"] = Json::UInt(p.mProducerId);
      }
      else
      {
        const auto id = static_cast<T::ProducerId>(requiredId(json, "producer"));
        T::ProducerInfo p;
        if (contentServer->getProducerInfoById(0, id, p) != 0)
          throw NotFound("Unknown producer: " + std::to_string(id));
        if (action == "update")
        {
          setProducerFields(fields, p);
          check(contentServer->setProducerInfo(0, p), "Updating the producer");
        }
        else
          check(contentServer->deleteProducerInfoById(0, id), "Deleting the producer");
        result["id"] = Json::UInt(id);
      }
    }
    else if (type == "generation")
    {
      if (action == "add")
      {
        const auto producerId = static_cast<T::ProducerId>(requiredId(json, "producer"));
        T::ProducerInfo p;
        if (contentServer->getProducerInfoById(0, producerId, p) != 0)
          throw NotFound("Unknown producer: " + std::to_string(producerId));
        T::GenerationInfo g;
        g.mProducerId = producerId;
        setGenerationFields(fields, g);
        if (g.mName.empty() || g.mAnalysisTime.empty())
          throw BadRequest("A generation needs a name and an analysis time");
        check(contentServer->addGenerationInfo(0, g), "Adding the generation");
        result["id"] = Json::UInt(g.mGenerationId);
      }
      else
      {
        const auto id = static_cast<T::GenerationId>(requiredId(json, "generation"));
        T::GenerationInfo g;
        if (contentServer->getGenerationInfoById(0, id, g) != 0)
          throw NotFound("Unknown generation: " + std::to_string(id));
        if (action == "update")
        {
          setGenerationFields(fields, g);
          check(contentServer->setGenerationInfo(0, g), "Updating the generation");
        }
        else
          check(contentServer->deleteGenerationInfoById(0, id), "Deleting the generation");
        result["id"] = Json::UInt(id);
      }
    }
    else if (type == "file")
    {
      if (action == "add")
      {
        const auto generationId = static_cast<T::GenerationId>(requiredId(json, "generation"));
        T::GenerationInfo g;
        if (contentServer->getGenerationInfoById(0, generationId, g) != 0)
          throw NotFound("Unknown generation: " + std::to_string(generationId));
        T::FileInfo f;
        f.mProducerId = g.mProducerId;
        f.mGenerationId = generationId;
        setFileFields(fields, f);
        if (f.mName.empty())
          throw BadRequest("A file needs a name");
        check(contentServer->addFileInfo(0, f), "Adding the file");
        result["id"] = Json::UInt(f.mFileId);
      }
      else
      {
        const auto id = static_cast<T::FileId>(requiredId(json, "file"));
        T::FileInfo f;
        if (contentServer->getFileInfoById(0, id, f) != 0)
          throw NotFound("Unknown file: " + std::to_string(id));
        if (action == "update")
        {
          setFileFields(fields, f);
          check(contentServer->setFileInfo(0, f), "Updating the file");
        }
        else
          check(contentServer->deleteFileInfoById(0, id), "Deleting the file");
        result["id"] = Json::UInt(id);
      }
    }
    else if (type == "content")
    {
      const auto fileId = static_cast<T::FileId>(requiredId(json, "file"));
      const auto messageIndex = static_cast<T::MessageIndex>(requiredId(json, "message"));
      T::FileInfo f;
      if (contentServer->getFileInfoById(0, fileId, f) != 0)
        throw NotFound("Unknown file: " + std::to_string(fileId));

      if (action == "add")
      {
        T::ContentInfo c;
        c.mProducerId = f.mProducerId;
        c.mGenerationId = f.mGenerationId;
        c.mFileId = fileId;
        c.mFileType = f.mFileType;
        c.mMessageIndex = messageIndex;
        setContentFields(fields, c);
        check(contentServer->addContentInfo(0, c), "Adding the message");
      }
      else
      {
        T::ContentInfo c;
        if (contentServer->getContentInfo(0, fileId, messageIndex, c) != 0)
          throw NotFound("Unknown message: " + std::to_string(fileId) + " " + std::to_string(messageIndex));
        if (action == "update")
        {
          setContentFields(fields, c);
          check(contentServer->setContentInfo(0, c), "Updating the message");
        }
        else
          check(contentServer->deleteContentInfo(0, fileId, messageIndex), "Deleting the message");
      }
      result["file"] = Json::UInt(fileId);
      result["message"] = Json::UInt(messageIndex);
    }
    else
      throw BadRequest("'type' must be producer, generation, file or content");

    return result;
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (const NotFound&)
  {
    throw;
  }
  catch (const Forbidden&)
  {
    throw;
  }
  catch (const Rejected&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief Enables, disables or clears a log: {"server", "type", "action"}. */

Json::Value Plugin::api_logcontrol(const HTTP::Request& theRequest, SessionManagement::SessionInfo& session)
{
  try
  {
    if (!canControlLogs(session))
      throw Forbidden("Controlling the logs needs a member of the grid-admin group and logModificationEnabled in browser.flags");

    const Json::Value json = requestJson(theRequest);
    const std::string server = member(json, "server");
    const std::string type = member(json, "type");
    const std::string action = member(json, "action");

    Log* log = serverLog(*itsGridEngine, server, type);
    if (log == nullptr)
      throw NotFound("Unknown log: " + server + " " + type);

    if (action == "enable")
      log->enable();
    else if (action == "disable")
      log->disable();
    else if (action == "clear")
    {
      if (!isRegularFile(log->getFileName()))
        throw BadRequest("Only a log written to a regular file can be cleared: " + log->getFileName());
      log->clear();
    }
    else
      throw BadRequest("'action' must be enable, disable or clear");

    return logJson(log, server, type);
  }
  catch (const BadRequest&)
  {
    throw;
  }
  catch (const NotFound&)
  {
    throw;
  }
  catch (const Forbidden&)
  {
    throw;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/*! \brief The installed SmartMet libraries, engines and plugins with their sizes and
    modification times, as on the earlier Software page. */

Json::Value Plugin::api_software()
{
  try
  {
    const struct
    {
      const char* title;
      const char* directory;
      const char* pattern;
    } groups[] = {{"Libraries", "/usr/lib64", "libsmartmet*"},
                  {"Engines", "/usr/share/smartmet/engines", "*.so"},
                  {"Plugins", "/usr/share/smartmet/plugins", "*.so"}};

    Json::Value result(Json::objectValue);
    result["groups"] = Json::Value(Json::arrayValue);
    for (const auto& group : groups)
    {
      std::set<std::string> dirList;
      std::vector<std::pair<std::string, std::string>> fileList;
      std::vector<std::string> patterns = {group.pattern};
      getFileList(group.directory, patterns, false, dirList, fileList);
      std::sort(fileList.begin(), fileList.end(), [](const auto& a, const auto& b) { return a.second < b.second; });

      Json::Value files(Json::arrayValue);
      for (const auto& file : fileList)
      {
        const std::string path = file.first + "/" + file.second;
        Json::Value json(Json::objectValue);
        json["name"] = file.second;
        json["size"] = Json::Int64(getFileSize(path.c_str()));
        json["modified"] = timeString(getFileModificationTime(path.c_str()));
        files.append(json);
      }
      Json::Value g(Json::objectValue);
      g["title"] = group.title;
      g["directory"] = group.directory;
      g["files"] = files;
      result["groups"].append(g);
    }
    return result;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet
