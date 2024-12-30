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

#include "command_line_options.h"
#include "json.h"
#include "string_types.h"
#include "project.h"
#include "misc.h"
#include "new_project.h"
#include "logging.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <vector>

/**
 * @brief Will read the file and attempt to build it.
 * 
 * @param a_ProjectFilename 
 * @param a_Args 
 * @return int 
 */
static int BuildProjectFile(const std::string& a_ProjectFilename,appbuild::CommandLineOptions& a_Args)
{
	const bool verbose = a_Args.GetLoggingMode() >= appbuild::LOG_VERBOSE;

	if( verbose ){std::cout << "Loading project file " << a_ProjectFilename << "\n";}

    std::ifstream jsonFile(a_ProjectFilename);
    if( !jsonFile.is_open() )
    {
		if( verbose )
		{
			std::cout << "The project file \'" << a_ProjectFilename << "\' could not be loaded\n";
		}
		return EXIT_FAILURE;
    }

    std::stringstream jsonStream;
    jsonStream << jsonFile.rdbuf();// Read the whole file in...

	if( verbose ){std::cout << "Parsing project file " << a_ProjectFilename << "\n";}
	tinyjson::JsonProcessor projectFile(jsonStream.str());

	tinyjson::JsonValue projectRoot = projectFile.GetRoot();

	if( verbose ){std::cout << "Adding defaults to project file " << a_ProjectFilename << "\n";}
	appbuild::UpdateJsonProjectWithDefaults(projectRoot,verbose);

	// Validate the json.
	if( verbose ){std::cout << "Checking project file " << a_ProjectFilename << " against schema\n";}
	if( appbuild::ValidateJsonAgainstSchema(projectRoot,verbose) == false )
	{
		std::cout << "Project failed validation.\n";
		return EXIT_FAILURE;
	}

	const std::string projectPath = appbuild::GetPath(a_ProjectFilename);

	if( verbose ){std::cout << "Creating the project from file " << a_ProjectFilename << "\n";}
	appbuild::Project TheProject(projectRoot,a_ProjectFilename,projectPath,a_Args.GetNumThreads(),a_Args.GetLoggingMode(),a_Args.GetReBuild(),a_Args.GetTruncateOutput());
	if( TheProject )
	{
		TheProject.AddGenericFileDependency(a_ProjectFilename);
		if( a_Args.GetInteractiveMode() )
		{
			if( a_Args.ProcessInteractiveMode(TheProject) == false )
			{
				// User aborted interactive mode, so return.
				return EXIT_FAILURE;
			}
		}

		const std::string configname = a_Args.GetActiveConfig(TheProject.FindDefaultConfigurationName());

		if( a_Args.GetUpdatedProject() )
		{
			std::cout << "Updating project file and writing the results too \'" << a_Args.GetUpdatedOutputFileName() << "\'\n";

			std::ofstream file(a_Args.GetUpdatedOutputFileName());
			if( file.is_open() )
			{
				tinyjson::JsonValue jsonOutput;
				TheProject.Write(jsonOutput);
				tinyjson::JsonWriter(file,jsonOutput,true);
			}
			else
			{
				std::cout << "Failed to open json file  " << a_Args.GetUpdatedOutputFileName() << " for writing\n";
				return EXIT_FAILURE;
			}
		}// This is an if else as after loading, if the project is to be updated with the defaults, then we do not want to then also build.
		else if( configname.size() > 0 )
		{
			const std::chrono::system_clock::time_point build_start = std::chrono::system_clock::now();
			if( TheProject.Build(configname) )
			{
				if( a_Args.GetTimeBuild() )
				{
					std::cout << "Build took: " << appbuild::GetTimeDifference(build_start,std::chrono::system_clock::now()) << std::endl;
				}

				if( a_Args.GetRunAfterBuild() )
				{
					TheProject.RunOutputFile(configname);
				}
			}
			else
			{
				std::cout << "Build failed for project file \'" << a_ProjectFilename << "\'\n";
				return EXIT_FAILURE;
			}
		}
		else
		{
			std::cout << "No configuration found in the project file \'" << a_ProjectFilename << "\'\n";
			return EXIT_FAILURE;
		}
	}
	else
	{
		std::cout << "There was an error in the project file \'" << a_ProjectFilename << "\'\n";
		return EXIT_FAILURE;
	}

	// Get here, all ok.
	return EXIT_SUCCESS;
}

/**
 * @brief Main entry point for out app. Normal c/c++ stuff.
 * 
 * @param argc 
 * @param argv 
 * @return int EXIT_SUCCESS if all ok or EXIT_FAILURE if there was an issue.
 */
int main(int argc, char *argv[])
{
#ifdef DEBUG_BUILD
    std::cout << "argc " << argc << std::endl;
    for(int n = 0 ; n < argc ; n++ )
    {
		std::cout << "argv[" << n << "] " << argv[n] << std::endl;
    }

    std::cout << std::endl << "Runtime debug only unit tests.\n";
	assert( appbuild::DoMiscUnitTests() );
	std::cout << std::endl;
	std::cout << std::endl;

#endif

	appbuild::CommandLineOptions Args(argc,argv);

    if( Args.GetShowInformationOnly() )
	{
		if( Args.GetShowHelp() )
		{
			Args.PrintHelp();
		}

		if( Args.GetShowVersion() )
		{
			Args.PrintVersion();
		}

		if( Args.GetShowTypeSizes() )
		{
			Args.PrintTypeSizes();
		}

		if( Args.GetDisplayProjectSchema() )
		{
			Args.PrintProjectSchema();
		}
	}
	else if (Args.GetCreateNewProject() )
	{
		return appbuild::CreateNewProject(Args.GetNewProjectName(),Args.GetLoggingMode());
	}
	else if( Args.GetProjectFiles().size() > 0 )
	{
		for(const std::string& file : Args.GetProjectFiles() )
		{
			if( BuildProjectFile(file,Args) == EXIT_FAILURE )
			{
				Args.PrintGetHelp();
				return EXIT_FAILURE;
			}
		}
	}
	else
	{
		Args.PrintGetHelp();
	}

	return EXIT_SUCCESS;
}
