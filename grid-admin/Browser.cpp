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
    throw Spine::Exception(BCP, "Operation failed!", nullptr);
  }
}





Browser::~Browser()
{
}





void Browser::init(ConfigurationFile *theConfigurationFile,ContentServer_sptr theContentServer)
{
  try
  {
    itsContentServer = theContentServer;
    itsConfigurationFile = theConfigurationFile;
  }
  catch (...)
  {
    throw Spine::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_content(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
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
    output << "<H2>Content</H2>\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin?producerId=" << producerId << "\">Producers</A> / <A href=\"grid-admin?page=generations&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "\">" << producerName << "</A> / <A href=\"grid-admin?page=files&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "\">" << generationName << "</A> / " << fileId;
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
    output << "</TR>";


    T::ContentInfoList contentInfoList;
    T::ContentInfoList *cList = &contentInfoList;

    if (itsCachedFileId == fileId)
    {
      cList = &itsCachedContentInfoList;
    }
    else
    {
      itsContentServer->getContentListByFileId(0,fileId,contentInfoList);
      if (contentInfoList.getLength() > 1000)
      {
        itsCachedContentInfoList = contentInfoList;
        itsCachedFileId = fileId;
      }
    }

    std::string fg = "#000000";
    std::string bg = "#FFFFFF";

    uint len = cList->getLength();
    for (uint t=0; t<maxRecords; t++)
    {
      T::ContentInfo *content = cList->getContentInfoByIndex(startMessageIndex+t);

      if (content != nullptr)
      {
        nextMessageIndex = content->mMessageIndex+1;
        if (content->mMessageIndex == messageIndex)
        {
          fg = "#FFFFFF";
          bg = "#FF0000";
          output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << content->mMessageIndex << "&startMessageIndex=" << startMessageIndex <<"');\" >\n";
        }
        else
        {
          fg = "#000000";
          bg = "#FFFFFF";
          output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << content->mMessageIndex << "&startMessageIndex=" << startMessageIndex <<"');\" >\n";
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
        output << "<TD>"<< content->mModificationTime << "</TD>";
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
        output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex-maxRecords) << "');\" >&lt;&lt;</TD>\n";
      else
        output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

      if ((startMessageIndex+maxRecords) < len)
        output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << nextMessageIndex << "');\" >&gt;&gt;</TD>\n";
      else
        output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

      if (len > 100)
      {
        output << "<TD width=\"50\"> </TD>\n";

        if (startMessageIndex > 0)
        {
          if (startMessageIndex >= 100)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex-100) << "');\" >&lt;&lt;&lt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=0');\" >&lt;&lt;&lt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

        if ((startMessageIndex+maxRecords) < len)
        {
          if ((startMessageIndex+100) < len)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex+100) << "');\" >&gt;&gt;&gt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (len-maxRecords) << "');\" >&gt;&gt;&gt;</TD>\n";
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
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex-1000) << "');\" >&lt;&lt;&lt;&lt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=0');\" >&lt;&lt;&lt;&lt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

        if ((startMessageIndex+maxRecords) < len)
        {
          if ((startMessageIndex+1000) < len)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex+1000) << "');\" >&gt;&gt;&gt;&gt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (len-maxRecords) << "');\" >&gt;&gt;&gt;&gt;</TD>\n";
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
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex-10000) << "');\" >&lt;&lt;&lt;&lt;&lt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=0');\" >&lt;&lt;&lt;&lt;&lt;</TD>\n";
        }
        else
          output << "<TD width=\"70\" style=\"background:"+bg+";\" > </TD>\n";

        if ((startMessageIndex+maxRecords) < len)
        {
          if ((startMessageIndex+10000) < len)
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (startMessageIndex+10000) << "');\" >&gt;&gt;&gt;&gt;&gt;</TD>\n";
          else
            output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << fileId << "&startFileId=" << startFileId << "&messageIndex=" << messageIndex << "&startMessageIndex=" << (len-maxRecords) << "');\" >&gt;&gt;&gt;&gt;&gt;</TD>\n";
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
    throw Spine::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_files(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
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
    output << "<H2>Files</H2>\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin?producerId=" << producerId << "\">Producers</A> / <A href=\"grid-admin?page=generations&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "\">" << producerName << "</A> / " + generationName;
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

    std::string fg = "#000000";
    std::string bg = "#FFFFFF";

    T::FileInfoList fileInfoList;
    itsContentServer->getFileInfoListByGenerationId(0,generationId,startFileId,maxRecords,fileInfoList);

    uint len = fileInfoList.getLength();
    for (uint t=0; t<len; t++)
    {
      T::FileInfo *file = fileInfoList.getFileInfoByIndex(t);
      nextFileId = file->mFileId+1;

      if (file->mFileId == fileId)
      {
        fg = "#FFFFFF";
        bg = "#FF0000";
        output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" onClick=\"getPage(this,parent,'/grid-admin?page=content&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << file->mFileId << "&startFileId=" << startFileId << "');\" >\n";
      }
      else
      {
        fg = "#000000";
        bg = "#FFFFFF";
        output << "<TR style=\"background:" << bg <<"; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?page=files&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&fileId=" << file->mFileId << "&startFileId=" << startFileId << "');\" >\n";
      }


      output << "<TD>"<< file->mFileId << "</TD>";
      //output << "<TD>"<< (int)file->mFileType << "</TD>";
      output << "<TD>"<< file->mName << "</TD>";
      //output << "<TD>"<< file->mProducerId << "</TD>";
      //output << "<TD>"<< file->mGenerationId << "</TD>";
      //output << "<TD>"<< file->mGroupFlags << "</TD>";
      output << "<TD>"<< file->mFlags << "</TD>";
      output << "<TD>"<< file->mSourceId << "</TD>";
      output << "<TD>"<< file->mModificationTime << "</TD>";
      output << "<TD>"<< file->mDeletionTime << "</TD>\n";
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
        output << "<TD width=\"70\" align=\"center\" style=\"background:"+bg+";\" onmouseout=\"this.style='background:"+bg+";'\" onmouseover=\"this.style='background:#FFFF00;';\" onClick=\"getPage(this,parent,'/grid-admin?page=files&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generationId << "&generationName=" << generationName << "&startFileId=" << nextFileId << "');\" >&gt;&gt;</TD>\n";
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
    throw Spine::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_generations(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {

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
    output << "<H2>Generations</H2>\n";
    output << "<HR>\n";
    output << "<A href=\"grid-admin?producerId=" << producerId << "\">Producers</A> / " + producerName;
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
    output << "</TR>";

    std::string fg = "#000000";
    std::string bg = "#FFFFFF";

    T::GenerationInfoList generationInfoList;
    itsContentServer->getGenerationInfoListByProducerId(0,producerId,generationInfoList);

    uint len = generationInfoList.getLength();
    for (uint t=0; t<len; t++)
    {
      T::GenerationInfo *generation = generationInfoList.getGenerationInfoByIndex(t);

      if (generation->mGenerationId == generationId)
      {
        fg = "#FFFFFF";
        bg = "#FF0000";
        output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onClick=\"getPage(this,parent,'/grid-admin?page=files&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generation->mGenerationId << "&generationName=" << generation->mName << "');\" >\n";
      }
      else
      {
        fg = "#000000";
        bg = "#FFFFFF";
        output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg << "; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?page=generations&producerId=" << producerId << "&producerName=" << producerName << "&generationId=" << generation->mGenerationId << "&generationName=" << generation->mName << "');\" >\n";
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
    throw Spine::Exception(BCP, "Operation failed!", nullptr);
  }
}





bool Browser::page_producers(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
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
    output << "<H2>Producers</H2>\n";
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
    itsContentServer->getProducerInfoList(0,producerInfoList);

    uint len = producerInfoList.getLength();
    for (uint t=0; t<len; t++)
    {
      T::ProducerInfo *producer = producerInfoList.getProducerInfoByIndex(t);

      if (producer->mProducerId == producerId)
      {
        fg = "#FFFFFF";
        bg = "#FF0000";
        output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onClick=\"getPage(this,parent,'/grid-admin?page=generations&producerId=" << producer->mProducerId << "&producerName=" << producer->mName << "');\" >\n";
      }
      else
      {
        fg = "#000000";
        bg = "#FFFFFF";
        output << "<TR style=\"background:" << bg << "; color:" << fg << ";\" onmouseout=\"this.style='background:" << bg <<"; color:" << fg << ";'\" onmouseover=\"this.style='background:#FFFF00; color:#000000;';\" onClick=\"getPage(this,parent,'/grid-admin?page=producers&producerId=" << producer->mProducerId << "&producerName=" << producer->mName << "');\" >\n";
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
    throw Spine::Exception(BCP, "Operation failed!", nullptr);
  }
}




bool Browser::requestHandler(const Spine::HTTP::Request& theRequest,Spine::HTTP::Response& theResponse)
{
  try
  {
    auto page = theRequest.getParameter("page");

    if (!page || *page == "producers")
      return page_producers(theRequest,theResponse);

    if (*page == "generations")
      return page_generations(theRequest,theResponse);

    if (*page == "files")
      return page_files(theRequest,theResponse);

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
    throw Spine::Exception(BCP, "Operation failed!", nullptr);
  }
}



}  // namespace GridAdmin
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
