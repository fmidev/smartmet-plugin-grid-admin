#include "Browser.h"
#include <macgyver/TimeFormatter.h>
#include <macgyver/DateTime.h>
#include <grid-content/userManagement/implementation/ServiceImplementation.h>
#include <openssl/sha.h>
#include <random>
#include <cstdint>

namespace SmartMet
{

namespace Plugin
{
namespace GridAdmin
{


Browser::Browser()
{
  try
  {
    itsGridEngine = nullptr;
    itsAuthenticationRequired = false;
    itsBroswerSessionId = 0;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





Browser::~Browser()
{
}




void Browser::init(Engine::Grid::Engine* theGridEngine,bool authenticationRequired,std::string& groupsFile,std::string& usersFile)
{
  try
  {
    itsAuthenticationRequired = authenticationRequired;
    itsGroupsFile = groupsFile;
    itsUsersFile = usersFile;

    itsGridEngine = theGridEngine;

    if (itsAuthenticationRequired)
      UserManagement::localUserManagement.init(groupsFile.c_str(),usersFile.c_str());
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_engines(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY style=\"font-size:12;\">\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<HR>\n";
    output << "<H2>Engines</H2>\n";
    output << "<HR>\n";
    output << "    <OL>\n";

    if (itsGridEngine && itsGridEngine->isEnabled() && (session.mUserInfo.getUserId() == 0 || session.mUserInfo.hasPermission("grid-content-view")))
    {
      output << "      <LI>";
      output << "        <H4><A href=\"/grid-admin?target=grid-engine&page=start\">Grid Engine</A></H4>";
      itsGridEngine->browserContent(session,output);
      output << "      </LI>";
    }

    output << "    </OL>\n";
    output << "<HR>\n";
    output << "</BODY>\n";
    output << "</HTML>\n";


    theResponse.setContent(output.str());
    theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}




bool Browser::page_plugins(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY style=\"font-size:12;\">\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<HR>\n";
    output << "<H2>Plugins</H2>\n";
    output << "<HR>\n";
    output << "    <OL>\n";
    output << "      <LI>";
    output << "        <H4>Grid Admin</H4>";
    //output << "        <H4><A href=\"/grid-admin?&target=grid-gui-plugin&page=start\">Grid GUI</A></H4>";
    //itsGridEngine->browserContent(output);
    output << "      </LI>";
    output << "      <LI>";
    output << "        <H4>Grid GUI</H4>";
    //output << "        <H4><A href=\"/grid-admin?&target=grid-gui-plugin&page=start\">Grid GUI</A></H4>";
    //itsGridEngine->browserContent(output);
    output << "      </LI>";
    output << "    </OL>\n";
    output << "<HR>\n";


    output << "</BODY>\n";
    output << "</HTML>\n";


    theResponse.setContent(output.str());
    theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}




bool Browser::page_software(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::set<std::string> dirList;

    std::vector<std::pair<std::string,std::string>> fileList1;
    std::vector<std::string> filePatterns1;
    filePatterns1.push_back("libsmartmet*");
    getFileList("/usr/lib64",filePatterns1,false,dirList,fileList1);

    std::vector<std::pair<std::string,std::string>> fileList2;
    std::vector<std::string> filePatterns2;
    filePatterns2.push_back("*.so");
    getFileList("/usr/share/smartmet/engines",filePatterns2,false,dirList,fileList2);

    std::vector<std::pair<std::string,std::string>> fileList3;
    getFileList("/usr/share/smartmet/plugins",filePatterns2,false,dirList,fileList3);

    char fname[1000];
    char tmb[200];


    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY style=\"font-size:12;\">\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / \n";
    output << "<HR>\n";
    output << "<H2>Software</H2>\n";
    output << "<HR>\n";
    output << "    <OL>\n";
    output << "      <LI>";
    output << "        <H4>Libraries</H4>";

    output << "        <TABLE style=\"font-size:12;\">";
    for (auto it = fileList1.begin(); it != fileList1.end(); ++it)
    {
      sprintf(fname,"%s/%s",it->first.c_str(),it->second.c_str());
      time_t modTime= getFileModificationTime(fname);
      long long sz= getFileSize(fname);

      struct tm tt;
      localtime_tz(modTime, &tt, nullptr);
      sprintf(tmb,"%04d-%02d-%02d %02d:%02d:%02d",tt.tm_year + 1900,tt.tm_mon + 1,tt.tm_mday,tt.tm_hour,tt.tm_min,tt.tm_sec);
      output << "        <TR><TD width=\"200\">" << it->second << "</TD><TD width=\"120\" align=\"right\">" << sz << "</TD><TD <TD width=\"120\" align=\"right\">" << tmb << "</TD></TR>\n";
    }
    output << "        </TABLE>";

    output << "      </LI>";
    output << "      <LI>";
    output << "        <H4>Engines</H4>";
    output << "        <TABLE style=\"font-size:12;\">";
    for (auto it = fileList2.begin(); it != fileList2.end(); ++it)
    {
      sprintf(fname,"%s/%s",it->first.c_str(),it->second.c_str());
      time_t modTime= getFileModificationTime(fname);
      long long sz= getFileSize(fname);

      struct tm tt;
      localtime_tz(modTime, &tt, nullptr);
      sprintf(tmb,"%04d-%02d-%02d %02d:%02d:%02d",tt.tm_year + 1900,tt.tm_mon + 1,tt.tm_mday,tt.tm_hour,tt.tm_min,tt.tm_sec);
      output << "        <TR><TD width=\"200\">" << it->second << "</TD><TD width=\"120\" align=\"right\">" << sz << "</TD><TD <TD width=\"120\" align=\"right\">" << tmb << "</TD></TR>\n";
    }
    output << "        </TABLE>";
    output << "      </LI>";
    output << "      <LI>";
    output << "        <H4>Plugins</H4>";
    output << "        <TABLE style=\"font-size:12;\">";
    for (auto it = fileList3.begin(); it != fileList3.end(); ++it)
    {
      sprintf(fname,"%s/%s",it->first.c_str(),it->second.c_str());
      time_t modTime= getFileModificationTime(fname);
      long long sz= getFileSize(fname);

      struct tm tt;
      localtime_tz(modTime, &tt, nullptr);
      sprintf(tmb,"%04d-%02d-%02d %02d:%02d:%02d",tt.tm_year + 1900,tt.tm_mon + 1,tt.tm_mday,tt.tm_hour,tt.tm_min,tt.tm_sec);
      output << "        <TR><TD width=\"200\">" << it->second << "</TD><TD width=\"120\" align=\"right\">" << sz << "</TD><TD <TD width=\"120\" align=\"right\">" << tmb << "</TD></TR>\n";
    }
    output << "        </TABLE>";
    output << "      </LI>";
    output << "    </OL>\n";
    output << "<HR>\n";


    output << "</BODY>\n";
    output << "</HTML>\n";


    theResponse.setContent(output.str());
    theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}




void to_hex_string(uint8_t hash[20], char output[41])
{
  for (int i = 0; i < 20; i++)
    sprintf(output + i*2, "%02x", hash[i]);
  output[40] = '\0';
}



void Browser::countHash(const char *key,const char *password,char *hex)
{
  try
  {
    uint klen = strlen(key);
    uint plen = strlen(password);

    char data[plen+klen+5];
    memcpy(data,password,plen);
    memcpy(data+plen,key,klen);

    uint8_t hash[20];

    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data,klen+plen);
    SHA1_Final(hash, &ctx);

    to_hex_string(hash, hex);
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_login(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;
    auto target = theRequest.getParameter("target");
    auto page = theRequest.getParameter("page");

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t kk[3];
    for (uint t=0; t<3; t++)
      kk[t] = dist(gen);

    uchar *k = (uchar*)kk;
    char key[100];
    char *p = key;
    uint t=0;
    while (t<20)
    {
      char ch = (char)(32 + (k[t] % 96));
      if (isalnum(ch))
        p += sprintf(p,"%c",ch);

      t++;
    }

    session.setKey(key);


    if (!page || *page == "logout")
      page = "start";

    if (!target)
      target = "";

    output << "<HTML>\n";
    output << "<SCRIPT>\n";
    output << "function hash2(s)\n";
    output << "{\n";
    output << "  var key = '" << key << "';\n";
    output << "  var len = key.length;\n";
    output << "  var text = '';\n";
    output << "  for (let i = 0; i < s.length; i++) {\n";
    output << "    var c1 = s.charCodeAt(i);\n";
    output << "    var c2 = key.charCodeAt(i % len);\n";
    output << "    var val = (c1 * c2) % 256;\n";
    output << "    var hexValue = val.toString(16);\n";
    output << "    if (val < 16) text += '0';\n";
    output << "    text += hexValue;\n";
    output << "  } \n";
    output << "  return text;\n";
    output << "}\n";


    output << "function hash(str) {\n";
    output << "      key = \"" << key << "\";\n";
    output << "      st = str + key;\n";
    output << "      const msg = new TextEncoder().encode(st);\n";

    output << "      const H = [\n";
    output << "        0x67452301,\n";
    output << "        0xefcdab89,\n";
    output << "        0x98badcfe,\n";
    output << "        0x10325476,\n";
    output << "        0xc3d2e1f0\n";
    output << "      ];\n";

    output << "      const padded = new Uint8Array(((msg.length + 9 + 63) >> 6) << 6);\n";
    output << "      padded.set(msg);\n";
    output << "      padded[msg.length] = 0x80;\n";

    output << "      const bitLen = msg.length * 8;\n";
    output << "      const dv = new DataView(padded.buffer);\n";

    output << "      dv.setUint32(padded.length - 4, bitLen, false);\n";

    output << "      for (let i = 0; i < padded.length; i += 64) {\n";
    output << "        const w = new Uint32Array(80);\n";

    output << "        for (let j = 0; j < 16; j++) {\n";
    output << "          w[j] = dv.getUint32(i + j * 4, false);\n";
    output << "        }\n";

    output << "        for (let j = 16; j < 80; j++) {\n";
    output << "          w[j] = ((w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16]) << 1 | (w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16]) >>> 31) >>> 0;\n";
    output << "        }\n";

    output << "        let [a,b,c,d,e] = H;\n";

    output << "        for (let j = 0; j < 80; j++) {\n";
    output << "          let f, k;\n";

    output << "          if (j < 20) {\n";
    output << "            f = (b & c) | (~b & d);\n";
    output << "            k = 0x5a827999;\n";
    output << "          } else if (j < 40) {\n";
    output << "            f = b ^ c ^ d;\n";
    output << "            k = 0x6ed9eba1;\n";
    output << "          } else if (j < 60) {\n";
    output << "            f = (b & c) | (b & d) | (c & d);\n";
    output << "            k = 0x8f1bbcdc;\n";
    output << "          } else {\n";
    output << "            f = b ^ c ^ d;\n";
    output << "            k = 0xca62c1d6;\n";
    output << "          }\n";

    output << "          const temp = (((a << 5) | (a >>> 27)) + f + e + k + w[j]) >>> 0;\n";
    output << "          e = d;\n";
    output << "          d = c;\n";
    output << "          c = ((b << 30) | (b >>> 2)) >>> 0;\n";
    output << "          b = a;\n";
    output << "          a = temp;\n";
    output << "        }\n";

    output << "        H[0] = (H[0] + a) >>> 0;\n";
    output << "        H[1] = (H[1] + b) >>> 0;\n";
    output << "        H[2] = (H[2] + c) >>> 0;\n";
    output << "        H[3] = (H[3] + d) >>> 0;\n";
    output << "        H[4] = (H[4] + e) >>> 0;\n";
    output << "      }\n";

    output << "      return H.map(x => x.toString(16).padStart(8, \"0\")).join(\"\");\n";
    output << "    }\n";




    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "function login(obj,frm,url)\n";
    output << "{\n";
    output << "  var xhr = new XMLHttpRequest();\n";
    output << "  xhr.open(\"POST\", url);\n";
    output << "  xhr.setRequestHeader(\"Authorization\", \"Grid-Admin\");\n";
    output << "  xhr.setRequestHeader(\"Content-Type\", \"application/x-www-form-urlencoded\");\n";

    output << "  xhr.onreadystatechange = function ()\n";
    output << "  {\n";
    output << "    if (xhr.readyState === 4)\n";
    output << "    {\n";
    output << "      console.log(xhr.status, xhr.responseText);\n";
    output << "      if (xhr.status === 200) {;\n";
    output << "         getPage(obj,frm,url);\n";
    output << "      }\n";
    output << "    }\n";
    output << "  };\n";
    output << "  var user = document.getElementById('username').value;\n";
    output << "  var pw = document.getElementById('password').value;\n";
    output << "  var data = user + \":\" + hash(pw+pw+pw);\n";
    output << "  console.log(\"Sending:\", data);\n";
    output << "  xhr.send(data);";
    output << "}\n";

    output << "</SCRIPT>\n";
    output << "<BODY style=\"font-size:12;\">\n";
    output << "<HR>\n";
    output << "<H2>SmartMet Server</H2>\n";
    output << "<HR>\n";
    output << "<H2>Login</H2>\n";
    output << "<TABLE border=\"1\" width=\"400\" style=\"font-size:12;\">\n";
    output << "<TR><TD width=\"140\" bgColor=\"#D0D0D0\">Username</TD><TD><INPUT style=\"width:100%;\"  type=\"text\" id=\"username\" value=\"\"></TD></TR>\n";
    output << "<TR><TD width=\"140\" bgColor=\"#D0D0D0\">Password</TD><TD><INPUT style=\"width:100%;\"  type=\"password\" id=\"password\" value=\"\" onkeydown=\"if (event.key == 'Enter'){login(this,parent,'/grid-admin?page="+ *page + "&target=" + *target + "');}else{}\"></TD></TR>\n";
    output << "</TABLE>\n";

    std::string bg = "#C0C0C0";
    output << "<HR>\n";
    output << "<TABLE style=\"font-size:12;\">\n";
    output << "<TR>\n";
    output << "<TD width=\"150\" height=\"25\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#0000FF; color:#FFFFFF';\" onClick=\"login(this,parent,'/grid-admin?page="+ *page + "&target=" + *target + "');\" >Login</TD>\n";
    output << "</TR>\n";
    output << "</TABLE>\n";

    output << "</BODY>\n";
    output << "</HTML>\n";


    theResponse.setContent(output.str());
    theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}



bool Browser::page_start(SessionManagement::SessionInfo& session,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    if (itsAuthenticationRequired)
    {
      output << "<SCRIPT>\n";
      output << "function getPage(obj,frm,url)\n";
      output << "{\n";
      output << "  frm.location.href=url;\n";
      output << "}\n";
      output << "</SCRIPT>\n";
    }

    output << "<BODY style=\"font-size:12;\">\n";
    output << "<HR>\n";
    output << "<H2>SmartMet Server</H2>\n";
    output << "<HR>\n";
    output << "<OL>\n";
    output << "  <LI>";
    output << "    <H4><A href=\"/grid-admin?&page=engines\">Engines</A></H4>";
    output << "    <OL>\n";

    if (itsGridEngine && itsGridEngine->isEnabled() && (session.mUserInfo.getUserId() == 0 || session.mUserInfo.hasPermission("grid-content-view")))
    {
      output << "      <LI>";
      output << "        <H4><A href=\"/grid-admin?&target=grid-engine&page=start\">Grid Engine</A></H4>";
      itsGridEngine->browserContent(session,output);
      output << "      </LI>";
    }

    output << "    </OL>\n";
    output << "  </LI>";
    output << "  <LI>";
    output << "    <H4><A href=\"/grid-admin?&page=plugins\">Plugins</A></H4>";
    output << "    <OL>\n";
    output << "      <LI>";
    output << "        <H4>Grid Admin</H4>";
    //output << "        <H4><A href=\"/grid-admin?&target=grid-gui-plugin&page=start\">Grid GUI</A></H4>";
    //itsGridEngine->browserContent(output);
    output << "      </LI>";
    output << "      <LI>";
    output << "        <H4>Grid GUI</H4>";
    //output << "        <H4><A href=\"/grid-admin?&target=grid-gui-plugin&page=start\">Grid GUI</A></H4>";
    //itsGridEngine->browserContent(output);
    output << "      </LI>";
    output << "    </OL>\n";
    output << "  </LI>";

    output << "  <LI>";
    output << "    <H4><A href=\"/grid-admin?&page=software\">Software</A></H4>";
    output << "  </LI>";
    output << "</OL>\n";
    output << "<HR>\n";

    if (itsAuthenticationRequired)
      output << "<TABLE style=\"font-size:12;\"><TR><TD width=\"150\" height=\"25\" align=\"center\" style=\"background:#C0C0C0;\" onmouseout=\"this.style='background:#C0C0C0;'\" onmouseover=\"this.style='background:#0000FF; color:#FFFFFF';\" onClick=\"getPage(this,parent,'/grid-admin?page=logout');\">Logout</TD></TR></TABLE>\n";

    output << "</BODY>\n";
    output << "</HTML>\n";


    theResponse.setContent(output.str());
    theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}




bool Browser::requestHandler(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    SessionManagement::SessionInfo sessionInfo;
    sessionInfo.setSessionId(0);
    T::SessionId sessionId = 0;

    // Fetching the cookien that contains session information.
    auto cookie = theRequest.getHeader("Cookie");

    if (cookie)
    {
      // Parsing the session id.
      std::vector<std::string> list;
      splitString(*cookie,';',list);
      if (list.size() > 0)
      {
        for (auto it = list.begin(); it != list.end(); ++it)
        {
          std::vector<std::string> p;
          splitString(*it,'=',p);
          if (p.size() == 2  &&  strstr(p[0].c_str(),"sessionId") != nullptr)
            sessionId = toUInt64(p[1]);
        }
      }
    }


    if (sessionId != 0)
    {
      // Fetching session information according to the sessionId.

      time_t currentTime = time(nullptr);
      if (SessionManagement::localSessionManagement.getSessionInfo(itsBroswerSessionId,sessionId,sessionInfo) == 0)
      {
        if (theRequest.getClientIP() != sessionInfo.getAddress())
          sessionId = 0;
        else
        if (currentTime > sessionInfo.getExpirationTime())
        {
          // The session is old. Removing the session.
          SessionManagement::localSessionManagement.deleteSession(itsBroswerSessionId,sessionId);
          sessionId = 0;
        }
        else
        {
          // The session is valid. Updating the last access time.
          SessionManagement::localSessionManagement.updateSessionAccessTime(itsBroswerSessionId,sessionId);
        }
      }
      else
      {
        // Unknown session id.
        sessionId = 0;
      }
    }

    if (sessionId == 0)
    {
      // Creating new session
      SessionManagement::localSessionManagement.createSession(itsBroswerSessionId,sessionInfo);
      sessionInfo.setAddress(theRequest.getClientIP().c_str());
      sessionId = sessionInfo.getSessionId();
    }

     //sessionInfo.print(std::cout,0,0);
     //std::cout << theRequest.getContent() << "\n";

    if (itsAuthenticationRequired && sessionInfo.mUserInfo.getUserId() == 0)
    {
      // Parsing the usename and password
      std::vector<std::string> list;
      splitString(theRequest.getContent(),':',list);
      if (list.size() == 2)
      {
        // Searching user according to the username.
        UserManagement::UserInfo user;
        if (UserManagement::localUserManagement.getUserInfoByUsername(itsBroswerSessionId,list[0].c_str(),user) == 0)
        {
          // Checking the password

          char hash[200];
          char pw[200];
          sprintf(pw,"%s%s%s",user.getPassword(),user.getPassword(),user.getPassword());
          countHash(sessionInfo.getKey(),pw,hash);

          //printf("HASH [%s][%s]][%s][%s]\n",hash,list[1].c_str(),sessionInfo.getKey(),pw);

          if (strcmp(hash,list[1].c_str()) == 0)
          {
            sessionInfo.mUserInfo = user;
            SessionManagement::localSessionManagement.updateSessionInfo(itsBroswerSessionId,sessionInfo);
          }
        }
      }
    }

    auto page = theRequest.getParameter("page");

    if (itsAuthenticationRequired && page && *page == "logout")
    {
      if (SessionManagement::localSessionManagement.getSessionInfo(itsBroswerSessionId,sessionId,sessionInfo) == 0)
      {
        SessionManagement::localSessionManagement.deleteSession(itsBroswerSessionId,sessionId);
        SessionManagement::SessionInfo newSessionInfo;
        sessionInfo = newSessionInfo;
        sessionInfo.setAddress(theRequest.getClientIP().c_str());
        SessionManagement::localSessionManagement.createSession(itsBroswerSessionId,sessionInfo);
        sessionId = sessionInfo.getSessionId();
      }
    }

    if (sessionId > 0)
    {
      // The session information is send to the client as "a cookie".
      std::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));
      Fmi::DateTime t_now = Fmi::SecondClock::universal_time();
      Fmi::DateTime t_expires = t_now + Fmi::Seconds(3600);
      std::string expiration = tformat->format(t_expires);
      theResponse.setHeader("Set-Cookie","sessionId=" + std::to_string(sessionId) + "; expires=" + expiration);
    }

    bool res = false;
    if (itsAuthenticationRequired && sessionInfo.mUserInfo.getUserId() == 0)
    {
      res = page_login(sessionInfo,theRequest,theResponse);
      SessionManagement::localSessionManagement.updateSessionInfo(itsBroswerSessionId,sessionInfo);
      return res;
    }


    auto v = theRequest.getParameter("target");
    if (v)
      sessionInfo.setAttribute("grid-admin","target",v->c_str());

    v = theRequest.getParameter("page");
    if (v)
      sessionInfo.setAttribute("grid-admin","page",v->c_str());

    std::string target = "";
    sessionInfo.getAttribute("grid-admin","target",target);


    if (target == "grid-engine" && itsGridEngine  &&  (sessionInfo.mUserInfo.getUserId() == 0 || sessionInfo.mUserInfo.hasPermission("grid-content-view")))
    {
      res = itsGridEngine->browserRequest(sessionInfo,theRequest,theResponse);
      SessionManagement::localSessionManagement.updateSessionInfo(itsBroswerSessionId,sessionInfo);
      return res;
    }


    if (!page || *page == "start")
    {
      res = page_start(sessionInfo,theRequest,theResponse);
      SessionManagement::localSessionManagement.updateSessionInfo(itsBroswerSessionId,sessionInfo);
      return res;
    }

    if (*page == "engines")
    {
      res = page_engines(sessionInfo,theRequest,theResponse);
      SessionManagement::localSessionManagement.updateSessionInfo(itsBroswerSessionId,sessionInfo);
      return res;
    }

    if (*page == "plugins")
    {
      res = page_plugins(sessionInfo,theRequest,theResponse);
      SessionManagement::localSessionManagement.updateSessionInfo(itsBroswerSessionId,sessionInfo);
      return res;
    }

    if (*page == "software")
    {
      res = page_software(sessionInfo,theRequest,theResponse);
      SessionManagement::localSessionManagement.updateSessionInfo(itsBroswerSessionId,sessionInfo);
      return res;
    }

    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY style=\"font-size:12;\">\n";

    output << "Unknown page : " << *page << "\n";
    output << "</BODY>\n";
    output << "</HTML>\n";


    theResponse.setContent(output.str());
    theResponse.setHeader("Content-Type", "text/html; charset=UTF-8");

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}



}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
