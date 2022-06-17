#include "Browser.h"
#include <macgyver/TimeFormatter.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>

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
    itsReactor = nullptr;
    itsAuthenticationRequired = false;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





Browser::~Browser()
{
}





void Browser::readUsers()
{
  try
  {
    if (itsUsersFile.empty())
      return;

    FILE *file = fopen(itsUsersFile.c_str(),"re");
    if (file == nullptr)
    {
      Fmi::Exception exception(BCP,"Cannot open the users file!");
      exception.addParameter("Filename",itsUsersFile);
      throw exception;
    }

    char st[1000];

    while (!feof(file))
    {
      if (fgets(st,1000,file) != nullptr  &&  st[0] != '#')
      {
        bool ind = false;
        char *field[100];
        uint c = 1;
        field[0] = st;
        char *p = st;
        while (*p != '\0'  &&  c < 100)
        {
          if (*p == '"')
            ind = !ind;

          if ((*p == ';'  || *p == '\n') && !ind)
          {
            *p = '\0';
            p++;
            field[c] = p;
            c++;
          }
          else
          {
            p++;
          }
        }

        if (c > 2)
        {
          std::string username;
          std::string password;
          std::string description;

          if (field[0][0] != '\0')
            username = field[0];

          if (field[1][0] != '\0')
            password = field[1];

          if (field[2][0] != '\0')
            description = field[2];

          itsUsers.insert(std::pair<std::string,std::string>(username,password));
        }
      }
    }
    fclose(file);
  }
  catch (...)
  {
    throw Fmi::Exception(BCP,"Operation failed!",nullptr);
  }
}





void Browser::init(Spine::Reactor* theReactor,bool authenticationRequired,std::string& usersFile)
{
  try
  {
    itsReactor = theReactor;
    itsAuthenticationRequired = authenticationRequired;
    itsUsersFile = usersFile;

    auto  engine = itsReactor->getSingleton("grid", NULL);
    if (engine)
    {
      itsGridEngine = reinterpret_cast<Engine::Grid::Engine*>(engine);
    }
    readUsers();
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_engines(std::string& sessionId,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
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

    if (itsGridEngine && itsGridEngine->isEnabled())
    {
      output << "      <LI>";
      output << "        <H4><A href=\"/grid-admin?target=grid-engine&page=start\">Grid Engine</A></H4>";
      itsGridEngine->browserContent(output);
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




bool Browser::page_plugins(std::string& sessionId,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
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




bool Browser::page_login(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;
    auto target = theRequest.getParameter("target");
    auto page = theRequest.getParameter("page");

    if (!page || *page == "logout")
      page = "start";

    if (!target)
      target = "";

    output << "<HTML>\n";
    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "function login(obj,frm,url)\n";
    output << "{\n";
    output << "  var xhr = new XMLHttpRequest();\n";
    output << "  xhr.open(\"POST\", url);\n";
    output << "  xhr.setRequestHeader(\"Authorization\", \"Bearer test\");\n";
    output << "  xhr.setRequestHeader(\"Content-Type\", \"text/ascii\");\n";

    output << "  xhr.onreadystatechange = function ()\n";
    output << "  {\n";
    output << "    if (xhr.readyState === 4)\n";
    output << "    {\n";
    output << "       getPage(obj,frm,url);\n";
    output << "    }\n";
    output << "  };\n";
    output << "  var user = document.getElementById('username').value;\n";
    output << "  var pw = document.getElementById('password').value;\n";
    output << "  var data = user + \":\" + pw;\n";
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
    output << "<TR><TD width=\"140\" bgColor=\"#D0D0D0\">Password</TD><TD><INPUT style=\"width:100%;\"  type=\"password\" id=\"password\" value=\"\"></TD></TR>\n";
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



bool Browser::page_start(std::string& sessionId,const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
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

    if (itsGridEngine && itsGridEngine->isEnabled())
    {
      output << "      <LI>";
      output << "        <H4><A href=\"/grid-admin?&target=grid-engine&page=start\">Grid Engine</A></H4>";
      itsGridEngine->browserContent(output);
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
    std::string sessionId = "";

    if (itsAuthenticationRequired)
    {
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
            {
              sessionId = p[1];
            }
          }
        }
        else
          sessionId = "";

        auto it = itsValidSessions.find(sessionId);
        if (it != itsValidSessions.end())
        {
          auto t = time(0);
          if ((t - it->second) > 600)
          {
            // The session is old. Removing the session.
            itsValidSessions.erase(it);
            sessionId = "";
          }
          else
          {
            // The session is valid. Updating the last access time.
            it->second = time(0);
          }
        }
        else
        {
          // Unknown session id.
          sessionId = "";
        }
      }


      if (sessionId == "")
      {
        // Parsing the usename and password
        std::vector<std::string> list;
        splitString(theRequest.getContent(),':',list);
        if (list.size() == 2)
        {
          // Searching user according to the username.
          auto user = itsUsers.find(list[0]);
          if (user != itsUsers.end())
          {
            // Checking the password
            if (user->second == list[1])
            {
              // Creation a new session.
              sessionId = std::to_string(time(0) * rand());
              itsValidSessions.insert(std::pair<std::string,time_t>(sessionId,time(0)));

              // The session information is send to the client as "a cookie".
              boost::shared_ptr<Fmi::TimeFormatter> tformat(Fmi::TimeFormatter::create("http"));
              boost::posix_time::ptime t_now = boost::posix_time::second_clock::universal_time();
              boost::posix_time::ptime t_expires = t_now + boost::posix_time::seconds(3600);
              std::string expiration = tformat->format(t_expires);
              theResponse.setHeader("Set-Cookie","sessionId=" + sessionId + "; expires=" + expiration);
            }
          }
        }
      }
    }

    auto page = theRequest.getParameter("page");

    if (itsAuthenticationRequired && page && *page == "logout")
    {
      auto it = itsValidSessions.find(sessionId);
      if (it != itsValidSessions.end())
      {
        itsValidSessions.erase(it);
        sessionId = "";
      }
    }

    if (itsAuthenticationRequired && sessionId == "")
      return page_login(theRequest,theResponse);

    auto target = theRequest.getParameter("target");
    if (target && *target == "grid-engine" && itsGridEngine)
      return itsGridEngine->browserRequest(theRequest,theResponse);


    if (!page || *page == "start")
      return page_start(sessionId,theRequest,theResponse);

    if (*page == "engines")
      return page_engines(sessionId,theRequest,theResponse);

    if (*page == "plugins")
      return page_plugins(sessionId,theRequest,theResponse);

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
