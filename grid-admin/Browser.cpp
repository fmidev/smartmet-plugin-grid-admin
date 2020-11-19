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
    itsConfigurationFile = nullptr;
    itsCachedFileId = 0;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





Browser::~Browser()
{
}





void Browser::init(ConfigurationFile *theConfigurationFile,ContentServer_sptr theMainContentServer,Spine::Reactor* theReactor)
{
  try
  {
    itsConfigurationFile = theConfigurationFile;
    itsReactor = theReactor;
    itsMainContentServer = theMainContentServer;

    auto  engine = itsReactor->getSingleton("grid", NULL);
    if (engine)
    {
      //throw Fmi::Exception(BCP, "The 'grid-engine' unavailable!");
      itsGridEngine = reinterpret_cast<Engine::Grid::Engine*>(engine);
      itsCacheContentServer = itsGridEngine->getContentServer_sptr();
    }
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::includeFile(std::ostringstream& output,const char *filename)
{
  try
  {
    FILE *file = fopen(filename,"r");
    if (file == nullptr)
      return false;

    char st[100000];
    while (!feof(file))
    {
      if (fgets(st,100000,file) != 0)
        output << st;
    }
    fclose(file);

    return true;
  }
  catch (...)
  {
    throw Fmi::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_contentList(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    ContentServer_sptr contentServer = itsMainContentServer;
    std::string sourceStr = "main";

    auto source = theRequest.getParameter("source");
    if (source  &&  *source == "cache")
    {
      contentServer = itsCacheContentServer;
      sourceStr = "cache";
    }

    uint startMessageIndex = 0;
    uint nextMessageIndex = 0;
    uint maxRecords = 30;

    uint fileId = 0;
    uint startFileId = 0;
    uint producerId = 0;
    std::string producerName;
    uint generationId = 0;
    std::string generationName;
    uint messageIndex = 0xFFFFFFFF;

    auto producer = theRequest.getParameter("producerId");
    if (producer)
      producerId = atoi(producer->c_str());

    auto name = theRequest.getParameter("producerName");
    if (name)
      producerName = *name;

    auto generation = theRequest.getParameter("generationId");
    if (generation)
      generationId = atoi(generation->c_str());

    auto gname = theRequest.getParameter("generationName");
    if (gname)
      generationName = *gname;

    auto file = theRequest.getParameter("fileId");
    if (file)
      fileId = atoi(file->c_str());

    auto sfileId = theRequest.getParameter("startFileId");
    if (sfileId)
      startFileId = atoi(sfileId->c_str());

    auto smsgId = theRequest.getParameter("startMessageIndex");
    if (smsgId)
      startMessageIndex = atoi(smsgId->c_str());

    auto msgId = theRequest.getParameter("messageIndex");
    if (msgId)
      messageIndex = atoi(msgId->c_str());

    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "</SCRIPT>\n";


    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=contentServer\">Content Server</A> / ";
    output << "<A href=\"grid-admin?page=contentInformation\">Content Information</A> / ";
    output << "<A href=\"grid-admin?source=" << sourceStr << "&page=producers&producerId=" << producerId << "\">Producers (" << sourceStr << ")</A> / <A href=\"grid-admin?source=" << sourceStr << "&page=generations&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "\">" << producerName << "</A> / <A href=\"grid-admin?source=" << sourceStr << "&page=files&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "\">" << generationName << "</A> / " << fileId;
    output << "<HR>\n";
    output << "<H2>Content</H2>\n";
    output << "<HR>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    // output << "<TD>FileId</TD>";
    //output << "<TD>FileType</TD>";
    output << "<TD>MessageIndex</TD>";
    output << "<TD>FilePosition</TD>";
    output << "<TD>MessageSize</TD>";
    //output << "<TD>ProducerId</TD>";
    //output << "<TD>GenerationId</TD>";
    output << "<TD>ForecastTime</TD>";
    output << "<TD>FmiParameterId</TD>";
    output << "<TD>FmiParameterName</TD>";
    output << "<TD>GribParameterId</TD>";
    output << "<TD>NewbaseParameterId</TD>";
    output << "<TD>NewbaseParameterName</TD>";
    output << "<TD>FmiParameterLevelId</TD>";
    output << "<TD>Grib1ParameterLevelId</TD>";
    output << "<TD>Grib2ParameterLevelId</TD>";
    output << "<TD>ParameterLevel</TD>";
    output << "<TD>FmiParameterUnits</TD>";
    //output << "<TD>GribParameterUnits</TD>";
    output << "<TD>ForecastType</TD>";
    output << "<TD>ForecastNumber</TD>";
    output << "<TD>GeometryId</TD>";
    //output << "<TD>GroupFlags</TD>";
    output << "<TD>Flags</TD>";
    output << "<TD>SourceId</TD>";
    output << "<TD>ModificationTime</TD>";
    output << "<TD>DeletionTime</TD>";
    output << "</TR>";


    T::ContentInfoList contentInfoList;
    T::ContentInfoList *cList = &contentInfoList;

    if (itsCachedFileId == fileId)
    {
      cList = &itsCachedContentInfoList;
    }
    else
    {
      contentServer->getContentListByFileId(0,fileId,contentInfoList);
      if (contentInfoList.getLength() > 1000)
      {
        itsCachedContentInfoList = contentInfoList;
        itsCachedFileId = fileId;
      }
    }


    uint len = cList->getLength();
    for (uint t=0; t<maxRecords; t++)
    {
      T::ContentInfo *content = cList->getContentInfoByIndex(startMessageIndex+t);

      if (content != nullptr)
      {
        std::string fg = "#000000";
        std::string bg = "#FFFFFF";

        if (content->mDeletionTime > 0 && content->mDeletionTime < (time(nullptr) + 120))
          fg = "#808080";

        nextMessageIndex = content->mMessageIndex+1;
        if (content->mMessageIndex == messageIndex)
        {
          if (content->mDeletionTime == 0 || content->mDeletionTime > (time(nullptr) + 120))
          {
            fg = "#FFFFFF";
            bg = "#FF0000";

            std::ostringstream url;
            url << "/grid-gui";
            url << "?page=main";
            url << "&presentation=Image";
            url << "&producerId=" << content->mProducerId;
            url << "&generationId=" << content->mGenerationId;
            url << "&geometryId=" << content->mGeometryId;
            url << "&projectionId=" << content->mGeometryId;
            url << "&parameterId=" << content->getFmiParameterName();
            url << "&levelId=" << toString(content->mFmiParameterLevelId);
            url << "&level=" << content->mParameterLevel;
            url << "&start=" << content->mForecastTime;
            url << "&fileId=" << content->mFileId;
            url << "&messageIndex=" << content->mMessageIndex;
            url << "&forecastType=" << content->mForecastType;
            url << "&forecastNumber=" << content->mForecastNumber;
            url << "&coordinateLines=Grey";
            url << "&landBorder=Grey";

            output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" onClick=\"window.open('" << url.str() << "','_blank');\" >\n";
          }
          else
          {
            bg = "#E0E0E0";
            output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" >\n";
          }
        }
        else
        {
          output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << content->mMessageIndex << "&startMessageIndex=" << startMessageIndex <<"');\" >\n";
        }

        //output << "<TD>"<< content->mFileId << "</TD>";
        //output << "<TD>"<< (int)content->mFileType << "</TD>";
        output << "<TD>"<< content->mMessageIndex << "</TD>";
        output << "<TD>"<< content->mFilePosition << "</TD>";
        output << "<TD>"<< content->mMessageSize << "</TD>";
        //output << "<TD>"<< content->mProducerId << "</TD>";
        //output << "<TD>"<< content->mGenerationId << "</TD>";
        output << "<TD>"<< content->mForecastTime << "</TD>";
        output << "<TD>"<< content->mFmiParameterId << "</TD>";
        output << "<TD>"<< content->getFmiParameterName() << "</TD>";
        output << "<TD>"<< content->mGribParameterId << "</TD>";
        output << "<TD>"<< content->mNewbaseParameterId << "</TD>";
        output << "<TD>"<< content->mNewbaseParameterName << "</TD>";
        output << "<TD>"<< (int)content->mFmiParameterLevelId << "</TD>";
        output << "<TD>"<< (int)content->mGrib1ParameterLevelId << "</TD>";
        output << "<TD>"<< (int)content->mGrib2ParameterLevelId << "</TD>";
        output << "<TD>"<< content->mParameterLevel << "</TD>";
        output << "<TD>"<< content->mFmiParameterUnits << "</TD>";
        //output << "<TD>"<< content->mGribParameterUnits << "</TD>";
        output << "<TD>"<< content->mForecastType << "</TD>";
        output << "<TD>"<< content->mForecastNumber << "</TD>";
        output << "<TD>"<< content->mGeometryId << "</TD>";
        //output << "<TD>"<< content->mGroupFlags << "</TD>";
        output << "<TD>"<< content->mFlags << "</TD>";
        output << "<TD>"<< content->mSourceId << "</TD>";
        if (content->mModificationTime > 0)
          output << "<TD>"<< utcTimeFromTimeT(content->mModificationTime) << "</TD>";
        else
          output << "<TD></TD>";

        if (content->mDeletionTime > 0)
          output << "<TD>"<< utcTimeFromTimeT(content->mDeletionTime) << "</TD>";
        else
          output << "<TD></TD>";

        output << "</TR>";
      }
    }
    output << "</TABLE>\n";
    output << "<HR>\n";

    if (maxRecords < len)
    {
      std::string bg = "#A0A0A0";

      output << "<TABLE><TR>\n";

      if (startMessageIndex >= maxRecords)
        output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex-maxRecords) << "');\" >&lt;&lt;</TD>\n";
      else
        output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

      if ((startMessageIndex+maxRecords) < len)
        output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << nextMessageIndex << "');\" >&gt;&gt;</TD>\n";
      else
        output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

      if (len > 100)
      {
        output << "<TD width=\"50\"> </TD>\n";

        if (startMessageIndex > 0)
        {
          if (startMessageIndex >= 100)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex-100) << "');\" >&lt;&lt;&lt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=0');\" >&lt;&lt;&lt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

        if ((startMessageIndex+maxRecords) < len)
        {
          if ((startMessageIndex+100) < len)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex+100) << "');\" >&gt;&gt;&gt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (len-maxRecords) << "');\" >&gt;&gt;&gt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

      }

      if (len > 1000)
      {
        output << "<TD width=\"50\"> </TD>\n";

        if (startMessageIndex > 0)
        {
          if (startMessageIndex >= 1000)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex-1000) << "');\" >&lt;&lt;&lt;&lt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=0');\" >&lt;&lt;&lt;&lt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

        if ((startMessageIndex+maxRecords) < len)
        {
          if ((startMessageIndex+1000) < len)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex+1000) << "');\" >&gt;&gt;&gt;&gt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (len-maxRecords) << "');\" >&gt;&gt;&gt;&gt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";
      }

      if (len > 10000)
      {
        output << "<TD width=\"50\"> </TD>\n";

        if (startMessageIndex > 0)
        {
          if (startMessageIndex >= 10000)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex-10000) << "');\" >&lt;&lt;&lt;&lt;&lt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=0');\" >&lt;&lt;&lt;&lt;&lt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

        if ((startMessageIndex+maxRecords) < len)
        {
          if ((startMessageIndex+10000) < len)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex+10000) << "');\" >&gt;&gt;&gt;&gt;&gt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (len-maxRecords) << "');\" >&gt;&gt;&gt;&gt;&gt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";
      }

      output << "</TR></TABLE>\n";
    }

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





bool Browser::page_files(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    ContentServer_sptr contentServer = itsMainContentServer;
    std::string sourceStr = "main";

    auto source = theRequest.getParameter("source");
    if (source  &&  *source == "cache")
    {
      contentServer = itsCacheContentServer;
      sourceStr = "cache";
    }

    uint generationId = 0;
    std::string generationName;
    uint fileId = 0;
    uint startFileId = 0;
    uint nextFileId = 0;
    uint maxRecords = 30;

    uint producerId = 0;
    std::string producerName;

    auto producer = theRequest.getParameter("producerId");
    if (producer)
      producerId = atoi(producer->c_str());

    auto name = theRequest.getParameter("producerName");
    if (name)
      producerName = *name;

    auto generation = theRequest.getParameter("generationId");
    if (generation)
      generationId = atoi(generation->c_str());

    auto gname = theRequest.getParameter("generationName");
    if (gname)
      generationName = *gname;

    auto file = theRequest.getParameter("fileId");
    if (file)
      fileId = atoi(file->c_str());

    auto sfileId = theRequest.getParameter("startFileId");
    if (sfileId)
      startFileId = atoi(sfileId->c_str());

    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "</SCRIPT>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=contentServer\">Content Server</A> / ";
    output << "<A href=\"grid-admin?page=contentInformation\">Content Information</A> / ";
    output << "<A href=\"grid-admin?source=" << sourceStr << "&page=producers&producerId=" << producerId << "\">Producers (" << sourceStr << ")</A> / <A href=\"grid-admin?source=" << sourceStr << "&page=generations&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "\">" << producerName << "</A> / " + generationName;
    output << "<HR>\n";
    output << "<H2>Files</H2>\n";
    output << "<HR>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Id</TD>";
    //output << "<TD>Type</TD>";
    output << "<TD>Name</TD>";
    //output << "<TD>ProducerId</TD>";
    //output << "<TD>GenerationId</TD>";
    //output << "<TD>GroupFlags</TD>";
    output << "<TD>Flags</TD>";
    output << "<TD>SourceId</TD>";
    output << "<TD>ModificationTime</TD>";
    output << "<TD>DeletionTime</TD>";
    output << "</TR>\n";

    T::FileInfoList fileInfoList;
    contentServer->getFileInfoListByGenerationId(0,generationId,startFileId,maxRecords,fileInfoList);

    uint len = fileInfoList.getLength();
    for (uint t=0; t<len; t++)
    {
      T::FileInfo *file = fileInfoList.getFileInfoByIndex(t);
      nextFileId = file->mFileId+1;

      std::string fg = "#000000";
      std::string bg = "#FFFFFF";

      if (file->mDeletionTime > 0 && file->mDeletionTime < (time(nullptr) + 120))
        fg = "#808080";

      if (file->mFileId == fileId)
      {
        fg = "#FFFFFF";
        bg = "#FF0000";
        output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=contentList&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << file->mFileId << "&startFileId=" << startFileId << "');\" >\n";
      }
      else
      {
        output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=files&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << file->mFileId << "&startFileId=" << startFileId << "');\" >\n";
      }


      output << "<TD>"<< file->mFileId << "</TD>";
      //output << "<TD>"<< (int)file->mFileType << "</TD>";
      output << "<TD>"<< file->mName << "</TD>";
      //output << "<TD>"<< file->mProducerId << "</TD>";
      //output << "<TD>"<< file->mGenerationId << "</TD>";
      //output << "<TD>"<< file->mGroupFlags << "</TD>";
      output << "<TD>"<< file->mFlags << "</TD>";
      output << "<TD>"<< file->mSourceId << "</TD>";
      if (file->mModificationTime > 0)
        output << "<TD>"<< utcTimeFromTimeT(file->mModificationTime) << "</TD>";
      else
        output << "<TD></TD>";

      if (file->mDeletionTime > 0)
        output << "<TD>"<< utcTimeFromTimeT(file->mDeletionTime) << "</TD>";
      else
        output << "<TD></TD>";
      output << "</TR>\n";
    }

    output << "</TABLE>\n";
    output << "<HR>\n";

    if (startFileId > 0 || len >= maxRecords)
    {
      std::string bg = "#A0A0A0";

      output << "<TABLE><TR>\n";

      //output << "<TD style=\"width:100;background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=files&generation=" << generationId << "&startFileId=" << prevStartFileId << "');\" >Previous page</TD>\n";
      //window.history.back();
      if (startFileId > 0)
        output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"window.history.back();\" >&lt;&lt</TD>\n";
      else
        output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

      if (len >= maxRecords)
        output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=files&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&startFileId=" << nextFileId << "');\" >&gt;&gt;</TD>\n";
      else
        output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

      output << "</TR></TABLE>\n";
    }
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





bool Browser::page_generations(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    ContentServer_sptr contentServer = itsMainContentServer;
    std::string sourceStr = "main";

    auto source = theRequest.getParameter("source");
    if (source  &&  *source == "cache")
    {
      contentServer = itsCacheContentServer;
      sourceStr = "cache";
    }

    uint producerId = 0;
    uint generationId  = 0;
    std::string producerName;

    auto producer = theRequest.getParameter("producerId");
    if (producer)
      producerId = atoi(producer->c_str());

    auto name = theRequest.getParameter("producerName");
    if (name)
      producerName = *name;

    auto generation = theRequest.getParameter("generationId");
    if (generation)
      generationId = atoi(generation->c_str());

    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "</SCRIPT>\n";


    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=contentServer\">Content Server</A> / ";
    output << "<A href=\"grid-admin?page=contentInformation\">Content Information</A> / ";
    output << "<A href=\"grid-admin?source=" << sourceStr << "&page=producers&producerId=" << producerId << "\">Producers (" << sourceStr << ")</A> / " + producerName;
    output << "<HR>\n";
    output << "<H2>Generations</H2>\n";
    output << "<HR>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Id</TD>";
    //output << "<TD>Type</TD>";
    //output << "<TD>ProducerId</TD>";
    output << "<TD>Name</TD>";
    output << "<TD>Description</TD>";
    output << "<TD>AnalysisTime</TD>";
    output << "<TD>Flags</TD>";
    output << "<TD>SourceId</TD>";
    output << "<TD>Status</TD>";
    output << "<TD>DeletionTime</TD>";
    output << "</TR>";

    T::GenerationInfoList generationInfoList;
    contentServer->getGenerationInfoListByProducerId(0,producerId,generationInfoList);

    uint len = generationInfoList.getLength();
    for (uint t=0; t<len; t++)
    {
      T::GenerationInfo *generation = generationInfoList.getGenerationInfoByIndex(t);

      std::string fg = "#000000";
      std::string bg = "#FFFFFF";

      if (generation->mDeletionTime > 0 && generation->mDeletionTime < (time(nullptr) + 120))
        fg = "#808080";

      if (generation->mGenerationId == generationId)
      {
        fg = "#FFFFFF";
        bg = "#FF0000";
        output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=files&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generation->mGenerationId << "&generationName=" << generation->mName << "');\" >\n";
      }
      else
      {
        output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg << "; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=generations&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generation->mGenerationId << "&generationName=" << generation->mName << "');\" >\n";
      }


      output << "<TD>"<< generation->mGenerationId << "</TD>";
      //output << "<TD>"<< generation->mGenerationType << "</TD>";
      //output << "<TD>"<< generation->mProducerId << "</TD>";
      output << "<TD>"<< generation->mName << "</TD>";
      output << "<TD>"<< generation->mDescription << "</TD>";
      output << "<TD>"<< generation->mAnalysisTime << "</TD>";
      output << "<TD>"<< generation->mFlags << "</TD>";
      output << "<TD>"<< generation->mSourceId << "</TD>";
      output << "<TD>"<< (int)generation->mStatus << "</TD>";
      if (generation->mDeletionTime > 0)
        output << "<TD>"<< utcTimeFromTimeT(generation->mDeletionTime) << "</TD>";
      else
        output << "<TD></TD>";
      output << "</TR>";
    }

    output << "</TABLE>\n";
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





bool Browser::page_producers(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    ContentServer_sptr contentServer = itsMainContentServer;
    std::string sourceStr = "main";

    auto source = theRequest.getParameter("source");
    if (source  &&  *source == "cache")
    {
      contentServer = itsCacheContentServer;
      sourceStr = "cache";
    }

    uint producerId = 0;
    auto producer = theRequest.getParameter("producerId");
    if (producer)
      producerId = atoi(producer->c_str());

    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "</SCRIPT>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=contentServer\">Content Server</A> / ";
    output << "<A href=\"grid-admin?page=contentInformation\">Content Information</A> / ";
    output << "<HR>\n";
    output << "<H2>Producers (" << sourceStr << ")</H2>\n";
    output << "<HR>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Id</TD>";
    output << "<TD>Name</TD>";
    output << "<TD>Title</TD>";
    output << "<TD>Description</TD>";
    output << "<TD>Flags</TD>";
    output << "<TD>SourceId</TD>";
    output << "</TR>";

    std::string fg = "#000000";
    std::string bg = "#FFFFFF";

    T::ProducerInfoList producerInfoList;
    contentServer->getProducerInfoList(0,producerInfoList);

    uint len = producerInfoList.getLength();
    for (uint t=0; t<len; t++)
    {
      T::ProducerInfo *producer = producerInfoList.getProducerInfoByIndex(t);

      if (producer->mProducerId == producerId)
      {
        fg = "#FFFFFF";
        bg = "#FF0000";
        output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=generations&producerId=" << producer->mProducerId << "&producerName=" << producer->mName << "');\" >\n";
      }
      else
      {
        fg = "#000000";
        bg = "#FFFFFF";
        output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?source=" << sourceStr << "&page=producers&producerId=" << producer->mProducerId << "&producerName=" << producer->mName << "');\" >\n";
      }


      output << "<TD>"<< producer->mProducerId << "</TD>";
      output << "<TD>"<< producer->mName << "</TD>";
      output << "<TD>"<< producer->mTitle << "</TD>";
      output << "<TD>"<< producer->mDescription << "</TD>";
      output << "<TD>"<< producer->mFlags << "</TD>";
      output << "<TD>"<< producer->mSourceId << "</TD>";
      output << "</TR>";
    }

    output << "</TABLE>\n";
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




bool Browser::page_contentInformation(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=contentServer\">Content Server</A> / ";
    output << "<HR>\n";
    output << "<H2>Content information</H2>\n";
    output << "<HR>\n";
    output << "<OL>\n";
    output << "  <LI>";
    output << "    <A href=\"/grid-admin?&page=producers&source=main\">Main content information (Redis)</A>";
    output << "  </LI>";
    output << "  <LI>";
    output << "     <A href=\"/grid-admin?&page=producers&source=cache\">Cached content information (Grid Engine)</A>";
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




bool Browser::page_contentServer(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<HR>\n";
    output << "<H2>Content Server</H2>\n";
    output << "<HR>\n";
    output << "<OL>\n";
    output << "  <LI>";
    output << "    <A href=\"/grid-admin?&page=contentInformation\">Content Information</A>";
    output << "    <OL>\n";
    output << "      <LI>";
    output << "        <A href=\"/grid-admin?&page=producers&source=main\">Main content information (Redis)</A>";
    output << "      </LI>";
    output << "      <LI>";
    output << "         <A href=\"/grid-admin?&page=producers&source=cache\">Cached content information (Grid Engine)</A>";
    output << "      </LI>";
    output << "    </OL>\n";
    output << "  </LI>";
    output << "  <LI>";
    output << "    <H4>Logs</H4>";
    output << "    <OL>\n";
    output << "      <LI>";
    output << "        Processing log";
    output << "      </LI>";
    output << "      <LI>";
    output << "        Debug log";
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




bool Browser::page_gridEngine_luaFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    auto hashStr = theRequest.getParameter("filename");
    if (!hashStr)
      return false;

    auto fname = itsFilenames.find(*hashStr);
    if (fname == itsFilenames.end())
      return false;


    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_luaFiles\">LUA files</A> / ";
    output << "<HR>\n";
    output << "<H2>" << fname->second << "</H2>\n";
    output << "<HR>\n";

    output << "<PRE style=\"background-color: #F0F0F0;\">\n";


    output << "\n";
    includeFile(output,fname->second.c_str());
    output << "\n";

    output << "</PRE>\n";
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




bool Browser::page_gridEngine_luaFiles(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    ConfigurationFile configurationFile;
    configurationFile.readFile(itsGridEngine->getConfigurationFileName().c_str());

    string_vec luaFiles;
    configurationFile.getAttributeValue("smartmet.engine.grid.query-server.luaFiles",luaFiles);

    string_vec luaFiles2;
    configurationFile.getAttributeValue("smartmet.engine.grid.data-server.luaFiles",luaFiles2);

    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "</SCRIPT>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<HR>\n";
    output << "<H2>LUA files</H2>\n";
    output << "<HR>\n";
    output << "<H3>Introduction</H3>\n";
    output << "<P>LUA files are used for the following purposes:</P> \n";
    output << "<OL>\n";
    output << "  <LI>\n";
    output << "    <B>Converting mapped parameters</B>\n";
    output << "      <P>Sometimes parameter mapping requires that the parameter values will be converted before they can be returned.\n";
    output << "      For example, the 'temperature' parameter (Celcius) is most likely mapped to a parameter that contains Kelvin values.\n";
    output << "      In this case, we have defined a conversion function (K2C or SUM) that is called in order to convert Kelvin values to \n";
    output << "      Celcius values. Conversion functions typically take only one grid parameter as input. In addition they might\n";
    output << "      take some location specific parameters (like dem, dark, daylength, etc.). The actual implementation of these \n";
    output << "      conversion functions are usually defined in LUA files.</P>\n";
    output << "  </LI>\n";
    output << "  <LI>\n";
    output << "    <B>Processing query parameters</B>\n";
    output << "      <P>LUA functions can be used for example in Timeseries queries. In this case the requested parameter values are\n";
    output << "      given as input to a LUA function (for example 'MYFUNC{Temperature;Pressure;WindSpeedMS}').</P>\n";
    output << "  </LI>\n";
    output << "  <LI>\n";
    output << "    <B>Creating virtual grids</B>\n";
    output << "      <P>LUA functions can be used also for creating virtual grids. The idea is that these functions take one or \n";
    output << "      several grids as an input and produce a new grid. This grid is created during the request and it is not stored \n";
    output << "      permanently. Notice that, these LUA files are actually used by the data server (while other are used by the query server),\n";
    output << "      and that's why they are configured separately.</P>\n";
    output << "  </LI>\n";
    output << "</OL>\n";

    output << "<P>It is also good to realize that the most common functions (K2C,K2F,SUM,DIV,MUL,AVG,MIN,MAX,etc) are implemented \n";
    output << "with C++ for performance reasons. This means in practice that the system first checks if there is C++ implementation \n";
    output << "available, before it calls a LUA function.</P>\n";

    output << "<HR>\n";
    output << "<H3>Files</H3>\n";
    output << "<P>The current installation contains the following lua files used by the query server:</P>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Filename</TD>";
    output << "</TR>";


    std::string fg = "#000000";
    std::string bg = "#FFFFFF";

    for (auto it = luaFiles.begin(); it != luaFiles.end(); ++it)
    {
      std::size_t hash = 0;
      boost::hash_combine(hash,*it);
      std::string hashStr = std::to_string(hash);
      if (itsFilenames.find(hashStr) == itsFilenames.end())
        itsFilenames.insert(std::pair<std::string,std::string>(hashStr,*it));

      fg = "#000000";
      bg = "#FFFFFF";
      output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?&page=gridEngine_luaFile&filename=" << hash << "');\" >\n";
      output << "<TD>"<< *it << "</TD>";
      output << "</TR>";

    }

    output << "</TABLE>\n";
    output << "<HR>\n";

    output << "<P>The current installation contains the following lua files used by the data server (for virtual grid creation):</P>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Filename</TD>";
    output << "</TR>";

    for (auto it = luaFiles2.begin(); it != luaFiles2.end(); ++it)
    {
      std::size_t hash = 0;
      boost::hash_combine(hash,*it);
      std::string hashStr = std::to_string(hash);
      if (itsFilenames.find(hashStr) == itsFilenames.end())
        itsFilenames.insert(std::pair<std::string,std::string>(hashStr,*it));

      fg = "#000000";
      bg = "#FFFFFF";
      output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?&page=gridEngine_luaFile&filename=" << hash << "');\" >\n";
      output << "<TD>"<< *it << "</TD>";
      output << "</TR>";

    }

    output << "</TABLE>\n";
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




bool Browser::page_gridEngine_parameterAliasFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    auto hashStr = theRequest.getParameter("filename");
    if (!hashStr)
      return false;

    auto fname = itsFilenames.find(*hashStr);
    if (fname == itsFilenames.end())
      return false;


    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_parameterAliasFiles\">Parameter alias files</A> / ";
    output << "<HR>\n";
    output << "<H2>" << fname->second << "</H2>\n";
    output << "<HR>\n";

    output << "<PRE style=\"background-color: #F0F0F0;\">\n";

    output << "\n";
    includeFile(output,fname->second.c_str());
    output << "\n";

    output << "</PRE>\n";
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




bool Browser::page_gridEngine_parameterAliasFiles(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    ConfigurationFile configurationFile;
    configurationFile.readFile(itsGridEngine->getConfigurationFileName().c_str());

    string_vec parameterAliasFiles;
    configurationFile.getAttributeValue("smartmet.engine.grid.query-server.aliasFiles",parameterAliasFiles);


    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "</SCRIPT>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<HR>\n";
    output << "<H2>Parameter alias files</H2>\n";
    output << "<HR>\n";
    output << "<H3>Introduction</H3>\n";
    output << "<P>Parameter alias files can be used in order to create alias names for query parameters (For example,\n";
    output << "'TemperatureK' => 'T-K').Alias names are very useful when the original parameter name is long or if it \n";
    output << "contains a function (For example, 'TemperatureC' => 'SUM{T-K;-273.15}'.</P>\n";

    output << "<P>Notice that usually you should use <B>parameter mapping files</B> in order to define new  \"official\" \n";
    output << "parameter names. Alias names can be used for defining shorter names for those \"official\" names. They can be \n";
    output << "used also for translating parameter names for different languages.</P>";

    output << "<HR>\n";
    output << "<H3>Files</H3>\n";
    output << "<P>The current installation contains the following parameter alias files:</P>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Filename</TD>";
    output << "</TR>";

    std::string fg = "#000000";
    std::string bg = "#FFFFFF";

    for (auto it = parameterAliasFiles.begin(); it != parameterAliasFiles.end(); ++it)
    {
      std::size_t hash = 0;
      boost::hash_combine(hash,*it);
      std::string hashStr = std::to_string(hash);
      if (itsFilenames.find(hashStr) == itsFilenames.end())
        itsFilenames.insert(std::pair<std::string,std::string>(hashStr,*it));

      fg = "#000000";
      bg = "#FFFFFF";
      output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?&page=gridEngine_parameterAliasFile&filename=" << hash << "');\" >\n";
      output << "<TD>"<< *it << "</TD>";
      output << "</TR>";

    }

    output << "</TABLE>\n";
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




bool Browser::page_gridEngine_producerMappingFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    auto hashStr = theRequest.getParameter("filename");
    if (!hashStr)
      return false;

    auto fname = itsFilenames.find(*hashStr);
    if (fname == itsFilenames.end())
      return false;


    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_producerMappingFiles\">Producer mapping files</A> / ";
    output << "<HR>\n";
    output << "<H2>" << fname->second << "</H2>\n";
    output << "<HR>\n";

    output << "<PRE style=\"background-color: #F0F0F0;\">\n";

    output << "\n";
    includeFile(output,fname->second.c_str());
    output << "\n";

    output << "</PRE>\n";
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




bool Browser::page_gridEngine_producerMappingFiles(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    ConfigurationFile configurationFile;
    configurationFile.readFile(itsGridEngine->getConfigurationFileName().c_str());

    string_vec producerMappingFiles;
    configurationFile.getAttributeValue("smartmet.engine.grid.query-server.producerMappingFiles",producerMappingFiles);


    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "</SCRIPT>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<HR>\n";
    output << "<H2>Producer mapping files</H2>\n";
    output << "<HR>\n";
    output << "<H3>Introduction</H3>\n";
    output << "<P>Producer mapping files are needed in order to make the new queries compatible with the older (newbase) queries. We do not ";
    output << "need these files if there are no compatibility requirements in place. </P>\n";

    output << "<P>The difficulty with the older queries is that the requested grid geometry and the level type is encoded into the producer name ";
    output << "(for example,'ecmwf_europe_surface'). In the new queries, same producers usually support multiple geometries and level types.";
    output << "In addition, it is quite common that there are no one-to-one mappings between old and new producers.\n";
    output << "It is more common that we need to do one-to-many mappings instead.</P>\n";

    output << "We need producer mapping files for the following purposes:\n";
    output << "<OL>\n";
    output << "  <LI>\n";
    output << "    <B>Mapping old (newbase) producers to new (Radon) producers, geometries and level types</B>\n";
    output << "      <P>Producer level mapping is used only if there are no better parameter level mappings available.\n";
    output << "      For example, the 'pal_scandinavia' producer is mapped to the producers 'SMARTMET' and 'SMARTMETMTA'.";
    output << "      However, this does not tell which producer supports the requested parameter and that's why the both";
    output << "      producers need to be queried. This might work, but not necessary as we like. That's because it is possible ";
    output << "      that the both producers have the same parameter (which might be counted differently).</P>";
    output << "  </LI>\n";
    output << "  <LI>\n";
    output << "    <B>Mapping old (newbase) parameters to correct new (Radon) producers</B>\n";
    output << "      <P>Usually all parameters should be mapped separately, because they might be found from different produces.\n";
    output << "      For example, the 'pal_scandinavia' producer has parameters 'Temperature' and 'PrecipitationType'. The first can be \n";
    output << "      found from the 'SMARTMET' producer, but the second is found from the 'SMARTMETMTA' producer. That's why they should have\n";
    output << "      own mappings.</P>\n";
    output << "  </LI>\n";
    output << "</OL>\n";
    output << "<HR>\n";
    output << "<H3>Files</H3>\n";
    output << "<P>The current installation contains the following producer mapping files:</P>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Filename</TD>";
    output << "</TR>";

    std::string fg = "#000000";
    std::string bg = "#FFFFFF";

    for (auto it = producerMappingFiles.begin(); it != producerMappingFiles.end(); ++it)
    {
      std::size_t hash = 0;
      boost::hash_combine(hash,*it);
      std::string hashStr = std::to_string(hash);
      if (itsFilenames.find(hashStr) == itsFilenames.end())
        itsFilenames.insert(std::pair<std::string,std::string>(hashStr,*it));

      fg = "#000000";
      bg = "#FFFFFF";
      output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?&page=gridEngine_producerMappingFile&filename=" << hash << "');\" >\n";
      output << "<TD>"<< *it << "</TD>";
      output << "</TR>";

    }

    output << "</TABLE>\n";
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




bool Browser::page_gridEngine_parameterMappingFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    auto hashStr = theRequest.getParameter("filename");
    if (!hashStr)
      return false;

    auto fname = itsFilenames.find(*hashStr);
    if (fname == itsFilenames.end())
      return false;


    const char* paramIdType[] = {"UNKNOWN","FMI-ID","FMI-NAME","GRIB-ID","NEWBASE-ID","NEWBASE-NAME","CDM-ID","CDM-NAME"};
    const char* levelIdType[] = {"UNKNOWN","FMI","GRIB1","GRIB2"};
    const char* interpolationMethod[] = {"None","Linear","Nearest","Min","Max","Logarithmic"};

    std::vector<std::vector<std::string>> records;
    readCsvFile(fname->second.c_str(),records);

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_parameterMappingFiles\">Parameter mapping files</A> / ";
    output << "<HR>\n";
    output << "<H2>"<< fname->second << "</H2>\n";
    output << "<HR>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Line</TD>";
    output << "<TD>Producer</TD>";
    output << "<TD>Mapping name</TD>";
    output << "<TD>Parameter Id type</TD>";
    output << "<TD>Parameter Id</TD>";
    output << "<TD>Geometry Id</TD>";
    output << "<TD>Level Id type</TD>";
    output << "<TD>Level Id</TD>";
    output << "<TD>Level</TD>";
    output << "<TD>Area interpolation</TD>";
    output << "<TD>Time interpolation</TD>";
    output << "<TD>Level interpolation</TD>";
    output << "<TD>Group flags</TD>";
    output << "<TD>Search match</TD>";
    output << "<TD>Mapping function</TD>";
    output << "<TD>Reverse mapping function</TD>";
    output << "<TD>Default precision</TD>";
    output << "</TR>";

    uint c = 0;
    for (auto rec = records.begin(); rec != records.end(); ++rec)
    {
      c++;
      output << "<TR>";
      output << "<TD>" << c << "</TD>";

      uint f = 0;
      for (auto field = rec->begin(); field != rec->end(); ++field)
      {
        f++;
        uint i = atoi(field->c_str());
        switch (f)
        {
          case 3:
            if (i < 8)
              output << "<TD>" << paramIdType[i] << "</TD>";
            else
              output << "<TD>" << *field << "</TD>";
            break;

          case 6:
            if (i < 4)
              output << "<TD>" << levelIdType[i] << "</TD>";
            else
              output << "<TD>" << *field << "</TD>";
            break;

          case 9:
            if (i < 5)
              output << "<TD>" << interpolationMethod[i] << "</TD>";
            else
              output << "<TD>" << *field << "</TD>";
            break;

          case 10:
            if (i < 5)
              output << "<TD>" << interpolationMethod[i] << "</TD>";
            else
              output << "<TD>" << *field << "</TD>";
            break;

          case 11:
            if (i < 6)
              output << "<TD>" << interpolationMethod[i] << "</TD>";
            else
              output << "<TD>" << *field << "</TD>";
            break;

          default:
            output << "<TD>" << *field << "</TD>";
            break;
        }
      }
      output << "</TR>\n";
    }

    output << "</TABLE>\n";
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




bool Browser::page_gridEngine_parameterMappingFiles(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    ConfigurationFile configurationFile;
    configurationFile.readFile(itsGridEngine->getConfigurationFileName().c_str());

    string_vec parameterMappingFiles;
    configurationFile.getAttributeValue("smartmet.engine.grid.query-server.mappingFiles",parameterMappingFiles);


    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<SCRIPT>\n";

    output << "function getPage(obj,frm,url)\n";
    output << "{\n";
    output << "  frm.location.href=url;\n";
    output << "}\n";

    output << "</SCRIPT>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<HR>\n";
    output << "<H2>Parameter mapping files</H2>\n";
    output << "<HR>\n";
    output << "<H3>Introduction</H3>\n";
    output << "<P>\n";
    output << "All grid parameters are registered into the content database (= Redis) by using parameter identifiers.\n";
    output << "Usually all parameters are registered by using the same parameter identifier type (FMI-NAME, Newbase-name, Grib-Id, etc.) \n";
    output << "and the content database is optimized for using this identifier type in its queries.\n";
    output << "If we want to use other parameter identifier types in our queries, we have to define parameter mapping files for them.\n";
    output << "On the other hand if we do not need other parameter identifiers, then we do not need so many mapping files.\n";
    output << "</P>\n";

    output << "We need parameter mapping files for the following purposes:\n";
    output << "<OL>\n";
    output << "  <LI>\n";
    output << "    <B>Defining other parameter identifiers (like newbase names) that can be used in the queries</B>\n";
    output << "      <P>For example, if the content database uses FMI-NAME as the primary parameter identifier type, but we want to use newbase";
    output << "      parameter names instead, then we have to define parameter mappings for all newbase names that we want to use.</P>";
    output << "  </LI>\n";
    output << "  <LI>\n";
    output << "    <B>Defining parameter value conversions for the queries </B>\n";
    output << "      <P>It is good to realize that some parameter mappings require that parameter values are converted during";
    output << "      the query. For example, the newbase parameter 'Temperature' (Celsius) is usually mapped to FMI-parameter 'T-K' (Kelvin), ";
    output << "      which means that the parameter values need to be converted from Kelvin to Celsius. In this case the conversion function ";
    output << "      is defined in the current mapping.</P>\n";

    output << "      <P>The current mapping contains also the reverse conversion function. This function is used ";
    output << "      when the query is requesting isolines/isobands with specific values. In this case the requested Celsius values are converted ";
    output << "      to Kelvins, before isolines/isobands are counted from the grid. This is faster than converting the whole grid to Celcius.</P>";
    output << "  </LI>\n";
    output << "  <LI>\n";
    output << "    <B>Defining parameter seach orders for the queries</B>\n";
    output << "      <P>It is quite common that queries do not exactly identify parameters that they are requesting. For example, they do not";
    output << "      necessary define which geometry, level type or level should be used. In this case, the first mapping of the requested parameter ";
    output << "       which 'search type' field has value 'E' is selected. So, the order of the mappings is significant in these cases.</P>";
    output << "  </LI>\n";
    output << "  <LI>\n";
    output << "    <B>Defining additional query information</B>\n";
    output << "      <P>It is qood to realize that there is also mapping file for FMI-NAME parameters in spite of that the content database uses ";
    output << "      usually this parameter identifier type as the primary parameter identifier. In this case the mappings are needed for defining ";
    output << "      additional information for the queries. By additional information, we mean the following fields:</P>";
    output << "      <OL>\n";
    output << "        <LI>Area interpolation method</LI>\n";
    output << "        <LI>Time interpolation method</LI>\n";
    output << "        <LI>Level interpolation method</LI>\n";
    output << "        <LI>Group flags (used for identifying climatological parameters)</LI>\n";
    output << "        <LI>Default precision</LI>\n";
    output << "      </OL></P>\n";
    output << "  </LI>\n";
    output << "  <LI>\n";
    output << "    <B>Defining missing parameter mappings automatically</B>\n";
    output << "      <P>New parameters are added continuosly into the content database. However, they cannot be queried if there are no mappings ";
    output << "      in place. That's why the system creates automatically temporary mappings for such parameters. Usually these mappings can be ";
    output << "      found from '*_auto.csv' files. These mappings are not necessary correct and that's why they need to be checked manually and ";
    output << "      moved into permanent mapping files.</P>";
    output << "  </LI>\n";
    output << "</OL>\n";

    output << "<HR>\n";
    output << "<H3>Files</H3>\n";
    output << "<P>The current installation contains the following parameter mapping files:</P>\n";

    output << "<TABLE border=\"1\" width=\"100%\">\n";
    output << "<TR bgColor=\"#D0D0D0\">";
    output << "<TD>Filename</TD>";
    output << "</TR>";

    std::string fg = "#000000";
    std::string bg = "#FFFFFF";

    for (auto it = parameterMappingFiles.begin(); it != parameterMappingFiles.end(); ++it)
    {
      std::size_t hash = 0;
      boost::hash_combine(hash,*it);
      std::string hashStr = std::to_string(hash);
      if (itsFilenames.find(hashStr) == itsFilenames.end())
        itsFilenames.insert(std::pair<std::string,std::string>(hashStr,*it));

      fg = "#000000";
      bg = "#FFFFFF";
      output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?&page=gridEngine_parameterMappingFile&filename=" << hash << "');\" >\n";
      output << "<TD>"<< *it << "</TD>";
      output << "</TR>";

    }

    output << "</TABLE>\n";
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




bool Browser::page_gridEngine_producerFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<HR>\n";
    output << "<H2>Producer search file</H2>\n";
    output << "<HR>\n";
    output << "<H3>Introduction</H3>\n";
    output << "<P>The producer serach file has two purposes : </P>\n";
    output << "<OL>\n";
    output << "  <LI>\n";
    output << "    <B>Defining produces and geometries that can be searched</B>\n";
    output << "      <P>If the requested producer and geometry are not in the list, then the query returns nothing.</P>";
    output << "  </LI>\n";
    output << "  <LI>\n";
    output << "    <B>Defining the search order</B>\n";
    output << "      <P>The producer search file defines the search order in the case where no producers are given in ";
    output << "      the query. In this case, the first producer where the search location is inside the current geometry, is selected.</P>\n";
    output << "  </LI>\n";
    output << "</OL>\n";
    output << "<HR>\n";
    output << "<H3>File (" << itsGridEngine->getProducerFileName() << ")</H3>\n";

    output << "<PRE style=\"background-color: #F0F0F0;\">\n";

    output << "\n";
    includeFile(output,itsGridEngine->getProducerFileName().c_str());
    output << "\n";

    output << "</PRE>\n";
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





bool Browser::page_gridEngine_configurationFile(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine_configuration\">Configuration</A> / ";
    output << "<HR>\n";
    output << "<H2>Configuration file (" << itsGridEngine->getConfigurationFileName() << ")</H2>\n";
    output << "<HR>\n";

    output << "<PRE style=\"background-color: #F0F0F0;\">\n";

    output << "\n";
    includeFile(output,itsGridEngine->getConfigurationFileName().c_str());
    output << "\n";

    output << "</pre>\n";
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





bool Browser::page_gridEngine_configuration(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<A href=\"grid-admin?page=gridEngine\">Grid Engine</A> / ";
    output << "<HR>\n";
    output << "<H2>Configuration</H2>\n";
    output << "<HR>\n";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_configurationFile\">Configuration file</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_parameterMappingFiles\">Parameter mapping files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_producerMappingFiles\">Producer mapping files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_parameterAliasFiles\">Parameter alias files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_luaFiles\">LUA files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_producerFile\">Producer search file</A>";
    output << "              </LI>";
    output << "            </OL>\n";
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




bool Browser::page_gridEngine(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<A href=\"grid-admin?page=engines\">Engines</A> / ";
    output << "<HR>\n";
    output << "<H2>Grid Engine</H2>\n";
    output << "<HR>\n";
    output << "        <OL>\n";
    output << "          <LI>";
    output << "            <H4><A href=\"/grid-admin?&page=gridEngine_configuration\">Configuration</A></H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_configurationFile\">Configuration file</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_parameterMappingFiles\">Parameter mapping files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_producerMappingFiles\">Producer mapping files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_parameterAliasFiles\">Parameter alias files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_luaFiles\">LUA files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_producerFile\">Producer search file</A>";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4><A href=\"/grid-admin?&page=contentServer\">Content Server</A></H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4><A href=\"/grid-admin?&page=contentInformation\">Content Information</A></H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    <A href=\"/grid-admin?&page=producers&source=main\">Main content information (Redis)</A>";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    <A href=\"/grid-admin?&page=producers&source=cache\">Cached content information (Grid Engine)</A>";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4>Data Server</H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                 </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4>QueryServer</H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "        </OL>\n";
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




bool Browser::page_engines(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin\">SmartMet Server</A> / ";
    output << "<HR>\n";
    output << "<H2>Engines</H2>\n";
    output << "<HR>\n";
    output << "    <OL>\n";
    output << "      <LI>";
    output << "        <H4><A href=\"/grid-admin?&page=gridEngine\">Grid Engine</A></H4>";
    output << "        <OL>\n";
    output << "          <LI>";
    output << "            <H4><A href=\"/grid-admin?&page=gridEngine_configuration\">Configuration</A></H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_configurationFile\">Configuration file</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_parameterMappingFiles\">Parameter mapping files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_producerMappingFiles\">Producer mapping files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_parameterAliasFiles\">Parameter alias files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_luaFiles\">LUA files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_producerFile\">Producer search file</A>";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4><A href=\"/grid-admin?&page=contentServer\">Content Server</A></H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4><A href=\"/grid-admin?&page=contentInformation\">Content Information</A></H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    <A href=\"/grid-admin?&page=producers&source=main\">Main content information (Redis)</A>";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    <A href=\"/grid-admin?&page=producers&source=cache\">Cached content information (Grid Engine)</A>";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4>Data Server</H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                 </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4>QueryServer</H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "        </OL>\n";
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
    output << "<BODY>\n";
    output << "<HR>\n";
    output << "<H2>SmartMet Server</H2>\n";
    output << "<HR>\n";
    output << "<OL>\n";
    output << "  <LI>";
    output << "    <H4><A href=\"/grid-admin?&page=engines\">Engines</A></H4>";
    output << "    <OL>\n";
    output << "      <LI>";
    output << "        <H4><A href=\"/grid-admin?&page=gridEngine\">Grid Engine</A></H4>";
    output << "        <OL>\n";
    output << "          <LI>";
    output << "            <H4><A href=\"/grid-admin?&page=gridEngine_configuration\">Configuration</A></H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_configurationFile\">Configuration file</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_parameterMappingFiles\">Parameter mapping files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_producerMappingFiles\">Producer mapping files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_parameterAliasFiles\">Parameter alias files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_luaFiles\">LUA files</A>";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <A href=\"/grid-admin?&page=gridEngine_producerFile\">Producer search file</A>";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4><A href=\"/grid-admin?&page=contentServer\">Content Server</A></H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4><A href=\"/grid-admin?&page=contentInformation\">Content Information</A></H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    <A href=\"/grid-admin?&page=producers&source=main\">Main content information (Redis)</A>";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    <A href=\"/grid-admin?&page=producers&source=cache\">Cached content information (Grid Engine)</A>";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4>Data Server</H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                 </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "          <LI>";
    output << "            <H4>QueryServer</H4>";
    output << "            <OL>\n";
    output << "              <LI>";
    output << "                <H4>Logs</H4>";
    output << "                <OL>\n";
    output << "                  <LI>";
    output << "                    Processing log";
    output << "                  </LI>";
    output << "                  <LI>";
    output << "                    Debug log";
    output << "                  </LI>";
    output << "                </OL>\n";
    output << "              </LI>";
    output << "            </OL>\n";
    output << "          </LI>";
    output << "        </OL>\n";
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
    auto page = theRequest.getParameter("page");

    if (!page || *page == "start")
      return page_start(theRequest,theResponse);

    if (*page == "engines")
      return page_engines(theRequest,theResponse);

    if (*page == "gridEngine")
      return page_gridEngine(theRequest,theResponse);

    if (*page == "gridEngine_configuration")
      return page_gridEngine_configuration(theRequest,theResponse);

    if (*page == "gridEngine_configurationFile")
      return page_gridEngine_configurationFile(theRequest,theResponse);

    if (*page == "gridEngine_producerFile")
      return page_gridEngine_producerFile(theRequest,theResponse);

    if (*page == "gridEngine_parameterMappingFiles")
      return page_gridEngine_parameterMappingFiles(theRequest,theResponse);

    if (*page == "gridEngine_parameterMappingFile")
      return page_gridEngine_parameterMappingFile(theRequest,theResponse);

    if (*page == "gridEngine_parameterAliasFiles")
      return page_gridEngine_parameterAliasFiles(theRequest,theResponse);

    if (*page == "gridEngine_parameterAliasFile")
      return page_gridEngine_parameterAliasFile(theRequest,theResponse);

    if (*page == "gridEngine_producerMappingFiles")
      return page_gridEngine_producerMappingFiles(theRequest,theResponse);

    if (*page == "gridEngine_producerMappingFile")
      return page_gridEngine_producerMappingFile(theRequest,theResponse);

    if (*page == "gridEngine_luaFiles")
      return page_gridEngine_luaFiles(theRequest,theResponse);

    if (*page == "gridEngine_luaFile")
      return page_gridEngine_luaFile(theRequest,theResponse);

    if (*page == "contentServer")
      return page_contentServer(theRequest,theResponse);

    if (*page == "contentInformation")
      return page_contentInformation(theRequest,theResponse);

    if (*page == "producers")
      return page_producers(theRequest,theResponse);

    if (*page == "generations")
      return page_generations(theRequest,theResponse);

    if (*page == "files")
      return page_files(theRequest,theResponse);

    if (*page == "contentList")
      return page_contentList(theRequest,theResponse);

    if (*page == "content")
      return page_content(theRequest,theResponse);

    std::ostringstream output;

    output << "<HTML>\n";
    output << "<BODY>\n";

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
