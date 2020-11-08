/*
   Copyright (C) 2017, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef __COMMAND_LINE_OPTIONS_H__
#define __COMMAND_LINE_OPTIONS_H__

#include <vector>
#include <string>

namespace appbuild{
/////////////////////////////////////////////////////////////////////////
class Project;
class CommandLineOptions
{
public:
	CommandLineOptions(int argc, char *argv[]);

	bool GetShowInformationOnly()const{return mShowHelp || mShowVersion || mShowTypeSizes || mDisplayProjectSchema;}
	bool GetShowHelp()const{return mShowHelp;}
	bool GetShowVersion()const{return mShowVersion;}
	bool GetShowTypeSizes()const{return mShowTypeSizes;}
	bool GetRunAfterBuild()const{return mRunAfterBuild;}
	bool GetReBuild()const{return mReBuild;}
	bool GetTimeBuild()const{return mTimeBuild;}
	bool GetUpdatedProject()const{return GetUpdatedOutputFileName().size() > 0;}
	bool GetCreateNewProject()const{return mNewProjectName.size() > 0;}
	bool GetInteractiveMode()const{return mInteractiveMode;}
	bool GetProjectIsEmbedded()const{return GetProjectJsonKey().size() > 0;}
	bool GetDisplayProjectSchema()const{return mDisplayProjectSchema;}
	bool GetWriteSchemaToFile()const{return GetSchemaSaveFilename().size() > 0;}

	int GetLoggingMode()const{return mLoggingMode;}
	int GetNumThreads()const{return mNumThreads;}
	int GetTruncateOutput()const{return mTruncateOutput;}
	std::vector<std::string> GetProjectFiles()const{return mProjectFiles;}

	/**
	 * @brief Get the Active Config name
	 * 
	 * @param pDefault A name to use if the user did not supply one.
	 * @return const std::string& 
	 */
	const std::string& GetActiveConfig(const std::string& pDefault)const
	{
		return mActiveConfig.size() > 0 ? mActiveConfig : pDefault;
	}

	const std::string& GetUpdatedOutputFileName()const{return mUpdatedOutputFileName;}
	const std::string& GetNewProjectName()const{return mNewProjectName;}

	const std::string& GetProjectJsonKey()const{return mProjectJsonKey;}
	const std::string& GetSchemaSaveFilename()const{return mSchemaSaveFilename;}

	void PrintHelp()const;
	void PrintVersion()const;
	void PrintTypeSizes()const;
	void PrintGetHelp()const;
	void PrintProjectSchema()const;

	/**
	 * @brief Handles the menu interactions of the 'limited' interactive mode.
	 * 
	 * @param pTheProject The project used for the selections being asked for.
	 * @return true Continue with the selection made.
	 * @return false They chose to quit the application.
	 */
	bool ProcessInteractiveMode(appbuild::Project& pTheProject);

private:
	bool mShowHelp;
	bool mShowVersion;
	bool mShowTypeSizes;
	bool mRunAfterBuild;
	bool mReBuild;
	bool mTimeBuild;
	bool mInteractiveMode;
	bool mDisplayProjectSchema;
	int mLoggingMode;
    int mNumThreads;
	int mTruncateOutput;
	std::vector<std::string> mProjectFiles;
	std::string mActiveConfig;
	std::string mUpdatedOutputFileName;
	std::string mNewProjectName;
	std::string mProjectJsonKey; //!< If set then the code is looking for a project embedded in other json. This is the key it will look for in the ROOT document.
	std::string mSchemaSaveFilename;	//!< The name of the file with write the project schema too, can be null, if so schema is written to standard out.
};

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild


#endif //__COMMAND_LINE_OPTIONS_H__
