#pragma once

#include <rapidxml_utils.hpp>
#include <unordered_map>
#include "SimpleMD5.h"

typedef rapidxml::file<>				RapidXML_File;
typedef rapidxml::xml_document<>		RapidXML_Doc;
typedef rapidxml::xml_node<>			RapidXML_Node;

class XMLWrapper
{
public:
	static XMLWrapper& GetInstance() { static XMLWrapper INSTANCE; return INSTANCE; }

	const RapidXML_Doc* LoadXMLFile( const char* filename )
	{
		RapidXML_File* newFile;
		auto hash = md5( std::string( filename ) );
		
		XMLListType::const_iterator findIter = m_LoadedXMLList.find( hash );
		if (findIter != m_LoadedXMLList.end()) newFile = (*findIter).second;
		else
		{
			MANAGE_MEMORY_NEW("XMLWrapper", sizeof(RapidXML_File));
			newFile = new RapidXML_File(filename);
		}

		MANAGE_MEMORY_NEW("XMLWrapper", sizeof(RapidXML_Doc));
		auto newDoc = new RapidXML_Doc;
		newDoc->parse<0>(newFile->data());
		
		m_LoadedXMLList[hash] = newFile;
		return newDoc;
	}

	bool RemoveXMLFile( const char* filename )
	{
		auto hash = md5( std::string( filename ) );
		XMLListType::const_iterator findIter = m_LoadedXMLList.find( hash );
		if (findIter != m_LoadedXMLList.end())
		{
			MANAGE_MEMORY_DELETE("XMLWrapper", sizeof(RapidXML_File));
			delete (*findIter).second;
			m_LoadedXMLList.erase(findIter);
			return true;
		}

		return false;
	}

	static void UnloadXMLDoc(const RapidXML_Doc* document)
	{
		MANAGE_MEMORY_DELETE("XMLWrapper", sizeof(RapidXML_Doc));
		delete document;
	}

	void Shutdown()
	{
		//  Unload all XML data
		while (!m_LoadedXMLList.empty())
		{
			MANAGE_MEMORY_DELETE("XMLWrapper", sizeof(RapidXML_File));
			delete m_LoadedXMLList.begin()->second;
			m_LoadedXMLList.erase(m_LoadedXMLList.begin());
		}
	}

private:
	XMLWrapper()	{}
	~XMLWrapper()	{}

	typedef std::unordered_map< std::string, RapidXML_File* > XMLListType;
	XMLListType m_LoadedXMLList;
};

//  Instance to be utilized by anyone including this header
XMLWrapper& xmlWrapper = XMLWrapper::GetInstance();