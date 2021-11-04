#include "Browser.h"

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
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





Browser::~Browser()
{
}





void Browser::init(Spine::Reactor* theReactor)
{
  try
  {
    itsReactor = theReactor;

    auto  engine = itsReactor->getSingleton("grid", NULL);
    if (engine)
    {
      itsGridEngine = reinterpret_cast<Engine::Grid::Engine*>(engine);
    }
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_engines(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
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




bool Browser::page_plugins(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
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




bool Browser::page_start(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
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
    auto target = theRequest.getParameter("target");

    if (target && *target == "grid-engine" && itsGridEngine)
      return itsGridEngine->browserRequest(theRequest,theResponse);


    auto page = theRequest.getParameter("page");

    if (!page || *page == "start")
      return page_start(theRequest,theResponse);

    if (*page == "engines")
      return page_engines(theRequest,theResponse);

    if (*page == "plugins")
      return page_plugins(theRequest,theResponse);

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
