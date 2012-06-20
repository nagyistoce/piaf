/***************************************************************************
	 file_video_acquisition_factory.h  - Factory for video acquisition from files

							 -------------------
	begin                : Tue May 01 2012
	copyright            : (C) 2012 by Christophe Seyve (CSE)
	email                : cseyve@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILE_VIDEO_ACQUISITION_FACTORY_H
#define FILE_VIDEO_ACQUISITION_FACTORY_H

#include "FileVideoAcquisition.h"


/** @brief Factory for video acquisition based on files

  The objects are registered by the file extensions they support
  */
class FileVideoAcquisitionFactory
{
public:
	typedef FileVideoAcquisition* (*CreatorFunction) (std::string path);
	typedef std::map<std::string, CreatorFunction> t_mapFactory;

	static FileVideoAcquisition * CreateInstance(std::string path)
	{
		std::string extension;
		int pos = path.find_last_of('.');
		extension = path.substr(pos+1);
		PIAF_MSG(SWLOG_INFO, "Opening '%s' => ext='%s'",
				 path.c_str(), extension.c_str());
		t_mapFactory::iterator it = get_mapFactory()->find(extension);
		if (it != get_mapFactory()->end())
		{
			if (it->second)
			{
				PIAF_MSG(SWLOG_DEBUG, "Found FileVideoAcquisition for ext='%s",
						 extension.c_str());
				return it->second(extension);
			}
		}

		PIAF_MSG(SWLOG_ERROR, "Found FileVideoAcquisition for ext='%s", extension.c_str());
		return NULL;
	}

	static std::string RegisterCreatorFunction(const std::string  & extension,
											   CreatorFunction classCreator)
	{
		PIAF_MSG(SWLOG_INFO, "Adding creator for extension '%s'", extension.c_str());
		get_mapFactory()->insert( std::pair<std::string, CreatorFunction>(extension, classCreator));
		return extension;
	}
protected:
	/// map where the construction info is stored
	/// to prevent inserting into map before initialisation takes place
	/// place it into static function as static member,
	/// so it will be initialised only once - at first call
	static t_mapFactory * get_mapFactory()
	{
		static t_mapFactory m_sMapFactory;
		return &m_sMapFactory;
	}
private:
};


#endif // FILE_VIDEO_ACQUISITION_FACTORY_H
