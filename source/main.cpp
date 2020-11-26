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

#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <vector>

#include "command_line_options.h"
#include "json.h"
#include "string_types.h"
#include "project.h"
#include "misc.h"
#include "she_bang.h"
#include "new_project.h"
#include "logging.h"


/**
 * @brief Will read the file and attempt to build it.
 * 
 * @param a_ProjectFilename 
 * @param a_Args 
 * @return int 
 */
static int BuildProjectFile(const std::string& a_ProjectFilename,appbuild::CommandLineOptions& a_Args)
{
	// Options read ok, now parse the project.
	// Read the project file.
	rapidjson::Document ProjectJson;

	if( appbuild::ReadJson(a_ProjectFilename,ProjectJson) == false )
	{
		std::cout << "The project file \'" << a_ProjectFilename << "\' could not be loaded" << std::endl;
		return EXIT_FAILURE;
	}

	rapidjson::Value& projectRoot = ProjectJson;

	// Is the project settings embedded in another file?
	if( a_Args.GetProjectIsEmbedded() )
	{
		if( projectRoot.HasMember(a_Args.GetProjectJsonKey()) )
		{
			projectRoot = projectRoot[a_Args.GetProjectJsonKey()];
		}
		else
		{
			std::cout << "The project was not found in Json object \'" << a_Args.GetProjectJsonKey() << "\' inside the file \'" << a_ProjectFilename << "\'" << std::endl;
			return EXIT_FAILURE;
		}
	}

	// Validate the json.
	if( appbuild::ValidateJsonAgainstSchema(projectRoot,a_Args.GetLoggingMode() == appbuild::LOG_VERBOSE ) == false )
	{
		// The validation failed, I wonder if they are using an embedded project?
		// If they are they should have added the option.
		// What I will do, to make life easier, see if they have a key called `appbuid`.
		// If they do then try again, if it still fails time to give up.
		if( projectRoot.HasMember("appbuild") && appbuild::ValidateJsonAgainstSchema(projectRoot["appbuild"],a_Args.GetLoggingMode() == appbuild::LOG_VERBOSE) )
		{
			std::cout << "Embedded project found on key 'appbuild' in the json file, using that. The root json file had filed the schema." << std::endl;
			std::cout << "Please consider using the --e or embedded-project option to silence this message." << std::endl;
			projectRoot = projectRoot["appbuild"];
		}
		else
		{// Did not find an embedded project, if there is one they need to tell us the key that they are using.
			std::cout << "A project can be in the root of other json data, that is embedded in the json file of another application." << std::endl;
			std::cout << "This allows you to state the key name in the root document where to find the appbuild project." << std::endl;
			std::cout << "Did you forget the --e or embedded-project option?" << std::endl;
			return EXIT_FAILURE;
		}
	}

	const std::string projectPath = appbuild::GetPath(a_ProjectFilename);
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
			std::cout << "Updating project file and writing the results too \'" << a_Args.GetUpdatedOutputFileName() << "\'" << std::endl;

			rapidjson::Document jsonOutput;
			jsonOutput.SetObject(); // Root object is an object not an array.
			TheProject.Write(jsonOutput);
			if( appbuild::SaveJson(a_Args.GetUpdatedOutputFileName(),jsonOutput) )
			{
				std::cout << "Project file updated" << std::endl;
			}
			else
			{
				std::cout << "Failed to write to the destination file." << std::endl;
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
				std::cout << "Build failed for project file \'" << a_ProjectFilename << "\'" << std::endl;
				return EXIT_FAILURE;
			}
		}
		else
		{
			std::cout << "No configuration found in the project file \'" << a_ProjectFilename << "\'" << std::endl;
			return EXIT_FAILURE;
		}
	}
	else
	{
		std::cout << "There was an error in the project file \'" << a_ProjectFilename << "\'" << std::endl;
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

    std::cout << std::endl << "Runtime debug only unit tests." << std::endl;
	assert( appbuild::DoMiscUnitTests() );
	std::cout << std::endl;
	std::cout << std::endl;

#endif

    // We have to special case the arg for using the app as a shebang.
    // This is because the getopt_long reorders commandline options.
    // So we have to see if argv[1] is set to -# and if so not call the CommandLineOptions class.
    // This code will assume that any args from index 3 onwards is for the exec.
    // argv[0] is the exec, argv[1] is the shebang option, argv[2] is the source file, added by the shell.
    if( argc > 2 && argv[1] && strcmp(argv[1],"-#") == 0 )
    {
#ifdef DEBUG_BUILD
		std::cout << "Running as a shebang!" << std::endl;
#endif
        return appbuild::BuildFromShebang(argc,argv);
    }

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
