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
#include <unistd.h>
#include <getopt.h>
#include <chrono>
#include <string.h>

#include "project.h"
#include "misc.h"
#include "string_types.h"
#include "json_writer.h"
#include "she_bang.h"
#include "logging.h"

class CommandLineOptions
{
public:
	CommandLineOptions(int argc, char *argv[]);

	bool GetShowHelp()const{return mShowHelp;}
	bool GetShowVersion()const{return mShowVersion;}
	bool GetRunAfterBuild()const{return mRunAfterBuild;}
	bool GetReBuild()const{return mReBuild;}
	bool GetTimeBuild()const{return mTimeBuild;}
	int GetLoggingMode()const{return mLoggingMode;}
	int GetNumThreads()const{return mNumThreads;}
	int GetTruncateOutput()const{return mTruncateOutput;}
	std::vector<std::string> GetProjectFiles()const{return mProjectFiles;}
	const std::string& GetActiveConfig()const{return mActiveConfig;}
	const std::string& GetUpdatedOutputFileName()const{return mUpdatedOutputFileName;}
	const bool GetUpdatedProject()const{return GetUpdatedOutputFileName().size() > 0;}

	void PrintHelp()const;
	void PrintVersion()const;
	void PrintGetHelp()const;

private:
	bool mShowHelp;
	bool mShowVersion;
	bool mRunAfterBuild;
	bool mReBuild;
	bool mTimeBuild;
	int mLoggingMode;
    int mNumThreads;
	int mTruncateOutput;
	std::vector<std::string> mProjectFiles;
	std::string mActiveConfig;
	std::string mUpdatedOutputFileName;
};

int main(int argc, char *argv[])
{
#ifdef DEBUG_BUILD
    std::cout << "argc " << argc << std::endl;
    for(int n = 0 ; n < argc ; n++ )
    {
		std::cout << "argv[" << n << "] " << argv[n] << std::endl;
    }
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

	CommandLineOptions Args(argc,argv);

    if( Args.GetShowHelp() )
	{
		Args.PrintHelp();
	}
	else if( Args.GetShowVersion() )
	{
		Args.PrintVersion();
	}
	else if( Args.GetProjectFiles().size() > 0 )
	{
		// Options read ok, now parse the project.
		for(const std::string& file : Args.GetProjectFiles() )
		{

			appbuild::Project TheProject(file,Args.GetNumThreads(),Args.GetLoggingMode(),Args.GetReBuild(),Args.GetTruncateOutput());
			if( TheProject )
			{
				const std::string configname = Args.GetActiveConfig();
				const appbuild::Configuration* ActiveConfig = TheProject.GetActiveConfiguration(configname);
				if( Args.GetUpdatedProject() )
				{
					std::cout << "Updating project file and writing the results too \'" << Args.GetUpdatedOutputFileName() << "\'" << std::endl;
					appbuild::JsonWriter JsonOutput;
					TheProject.Write(JsonOutput);

					std::ofstream updatedProjectFile(Args.GetUpdatedOutputFileName());
					if( updatedProjectFile )
					{
						updatedProjectFile << JsonOutput.Get();
					}
					else
					{
						std::cout << "Failed to write to the destination file." << std::endl;
					}
				}
				else if( ActiveConfig )
				{
					if( ActiveConfig->GetOk() )
					{
						const std::chrono::system_clock::time_point build_start = std::chrono::system_clock::now();
						if( TheProject.Build(ActiveConfig) )
						{
							if( Args.GetTimeBuild() )
							{
								std::cout << "Build took: " << appbuild::GetTimeDifference(build_start,std::chrono::system_clock::now()) << std::endl;
							}

							if( Args.GetRunAfterBuild() )
							{
								TheProject.RunOutputFile(ActiveConfig);
							}
						}
						else
						{
							std::cout << "Build failed for project file \'" << file << "\'" << std::endl;
						}
					}
					else
					{
						std::cout << "There is a problem with the configuration \'" << ActiveConfig->GetName() << "\' in the project file \'" << file << "\'. Please check the output for errors." << std::endl;
					}
				}
				else
				{
					std::cout << "No configuration found in the project file \'" << file << "\'" << std::endl;
				}
			}
			else
			{
				std::cout << "There was an error in the project file \'" << file << "\'" << std::endl;
				Args.PrintGetHelp();
			}
		}
	}
	else
	{
		Args.PrintGetHelp();
	}

	return EXIT_SUCCESS;
}


#define ARGUMENTS																																							\
		DEF_ARG(ARG_HELP,no_argument,					'h',"help","Display this help and exit.")																			\
		DEF_ARG(ARG_VERSION,no_argument,				'v',"version","Output version information and exit.")																\
		DEF_ARG(ARG_VERBOSE,no_argument,				'V',"verbose","Print more information about progress.")																\
		DEF_ARG(ARG_QUIET,no_argument,				    'q',"quiet","Print no information about progress, just errors will be displayed.")                       			\
		DEF_ARG(ARG_REBUILD,no_argument,				'r',"rebuild","Clean and rebuild all the source files.")															\
		DEF_ARG(ARG_RUN_AFTER_BUILD,no_argument,		'x',"run-after-build","If the build is successful then eXecute the app, but only if one project file submitted.")	\
		DEF_ARG(ARG_NUM_THREADS,required_argument,		'n',"num-threads","Sets the number of threads to use when tasks can be done in parallel.")							\
		DEF_ARG(ARG_ACTIVE_CONFIG,required_argument,	'c',"active-config","Builds the given configuration, if found.")													\
		DEF_ARG(ARG_UPDATE_PROJECT,required_argument,	'u',"update-project","Reads in the project file passed in then writes out an updated version with all the default paramiters\nfilled in that were not in the source.\nProject is not built if this option is specified.")	\
		DEF_ARG(ARG_TRUNCATE_OUTPUT,required_argument,	't',"truncate-output","Truncates the output to the first N lines, if you're getting too many errors this can help.")	\
		DEF_ARG(ARG_TIME_BUILD,no_argument,				'T',"time-build","Shows the total time of the build from start to finish.")												\
		DEF_ARG(ARG_SHEBANG,no_argument,				'#',"she-bang","Makes the c/c++ file with appbuild defined as a shebang run as if it was an executable. JIT Compiled.") \
		

enum eArguments
{
#define DEF_ARG(ARG_NAME,TAKES_ARGUMENT,ARG_SHORT_NAME,ARG_LONG_NAME,ARG_DESC)	ARG_NAME = ARG_SHORT_NAME,
	ARGUMENTS
#undef DEF_ARG
};

CommandLineOptions::CommandLineOptions(int argc, char *argv[]):
	mShowHelp(false),
	mShowVersion(false),
	mRunAfterBuild(false),
	mReBuild(false),
	mTimeBuild(false),
    mLoggingMode(appbuild::LOG_INFO),
	mNumThreads(std::thread::hardware_concurrency()),
	mTruncateOutput(0)
{
	std::string short_options;
#define DEF_ARG(ARG_NAME,TAKES_ARGUMENT,ARG_SHORT_NAME,ARG_LONG_NAME,ARG_DESC)	short_options += ARG_SHORT_NAME;if( TAKES_ARGUMENT == required_argument ){short_options+=":";}
	ARGUMENTS
#undef DEF_ARG

	struct option const long_options[] =
	{
#define DEF_ARG(ARG_NAME,TAKES_ARGUMENT,ARG_SHORT_NAME,ARG_LONG_NAME,ARG_DESC)	{ARG_LONG_NAME,TAKES_ARGUMENT,NULL,ARG_SHORT_NAME},
	ARGUMENTS
#undef DEF_ARG
		{NULL, 0, NULL, 0}
	};

	int c,oi;
	while( (c = getopt_long(argc,argv,short_options.c_str(),long_options,&oi)) != -1 )
	{
		switch(c)
		{
		case ARG_NUM_THREADS:
			if( optarg )
				mNumThreads = std::atoi(optarg);
			else
				std::cout << "Option -n (num-threads) was not passed correct value. -n 4 or --num-threads=4" << std::endl;
			break;

		case ARG_HELP:
			mShowHelp = true;
			break;

		case ARG_VERSION:
			mShowVersion = true;
			break;

		case ARG_VERBOSE:
			mLoggingMode = appbuild::LOG_VERBOSE;
			break;

		case ARG_QUIET:
			mLoggingMode = appbuild::LOG_ERROR;
			break;

		case ARG_RUN_AFTER_BUILD:
			mRunAfterBuild = true;
			break;

		case ARG_REBUILD:
			mReBuild = true;
			break;

		case ARG_TIME_BUILD:
			mTimeBuild = true;
			break;

		case ARG_ACTIVE_CONFIG:
			if( optarg )
				mActiveConfig = optarg;
			else
			{
				mShowHelp = true;
				std::cout << "Option -c (active-config) was not passed correct value. -c release or --active-config=release" << std::endl;
			}
			break;

		case ARG_UPDATE_PROJECT:
			if( optarg )
				mUpdatedOutputFileName = optarg;
			else
			{
				mShowHelp = true;
				std::cout << "Option -u (update-project) requires an ouput file name. -u file.proj or --active-config=file.proj" << std::endl;
			}
			break;

		case ARG_TRUNCATE_OUTPUT:
			if( optarg )
				mTruncateOutput = std::atoi(optarg);
			else
				std::cout << "Option -t (truncate-output) was not passed correct value. -t 10 or --num-threads=10 to show the first 10 lines of errors for each source file." << std::endl;
			break;

        case ARG_SHEBANG:
    		std::cout << "-# (she-bang) must be the first and oly option, it can not be used with ither options." << std::endl;
            mShowHelp = true;
			break;

		default:
			mShowHelp = true;
			break;
		}
	}

	// Do not continue if show help has been set.
	if( mShowHelp )
		return;

	// Don't need to show the help, so continue to see if we need to make some assumptions.
	while (optind < argc)
	{
		const std::string ProfileFile = argv[optind++];
		if( appbuild::FileExists(ProfileFile) )
			mProjectFiles.push_back(ProfileFile);
		else
		{
			std::cout << "Error: Project file " << ProfileFile << " is not found." << std::endl << "Current working dir " << appbuild::GetCurrentWorkingDirectory() << std::endl;
		}
	}

	if( mProjectFiles.size() == 0 )
	{
		// If no project files specified and not asking for help options, then auto find a proj file.
		// If only one is found use that, elso don't use any as it maybe not what they intended.
		const StringVec ProjFiles = appbuild::FindFiles(appbuild::GetCurrentWorkingDirectory(),"*.proj");
		if( ProjFiles.size() == 1 )
		{
			mProjectFiles = ProjFiles;
			if( mLoggingMode >= appbuild::LOG_VERBOSE )
				std::cout << "No project file specified, using \'" << mProjectFiles.front() << "\'"  << std::endl;
		}
		else if( ProjFiles.size() > 1 )
		{
			std::cout << "No project file specified, more than one in current working folder, can not continue. Please specify which projects to build." << std::endl;
			for(auto f : ProjFiles )
				std::cout << "    " << f << std::endl;
		}
	}

	if( GetRunAfterBuild() && mProjectFiles.size() > 1 )
	{
		std::cout << "Run after build option is only valid with one project file, ignoring" << std::endl;
		mRunAfterBuild = false;
	}
}


void CommandLineOptions::PrintHelp()const
{
	std::cout << "Usage: appbuild [OPTION]... [PROJECT FILE]..." << std::endl;
	std::cout << "Build one or more applications using the supplied project files." << std::endl << std::endl;
	std::cout << "Mandatory arguments to long options are mandatory for short options too." << std::endl << "Options:" << std::endl;

	// I build three vecs of strings for all options. So I can measure and then line up the text with spaces. Makes life easier when adding new options.
	StringVec ShortArgs;
	StringVec LongArgs;
	StringVec Descriptions;

#define DEF_ARG(ARG_NAME,TAKES_ARGUMENT,ARG_SHORT_NAME,ARG_LONG_NAME,ARG_DESC) if(true){ShortArgs.push_back(#ARG_SHORT_NAME);if(TAKES_ARGUMENT==required_argument){LongArgs.push_back(ARG_LONG_NAME "=arg");}else{LongArgs.push_back(ARG_LONG_NAME);}Descriptions.push_back(ARG_DESC);};
	ARGUMENTS
#undef DEF_ARG

	size_t DescMaxSpace = 0;
	for(auto lg : LongArgs)
	{
		size_t l = 5 + lg.size(); // 5 == 2 spaces + -X + 1 for space for short arg.
		if( DescMaxSpace < l )
			DescMaxSpace = l;
	}

	DescMaxSpace += 4; // Add 4 spaces for formatting.
	for(size_t n=0;n<ShortArgs.size();n++)
	{
		std::string line = "  -";
		line += ShortArgs[n][1];
		line += " ";
		line += LongArgs[n];
		line += " ";
		std::cout << line;

		size_t space = DescMaxSpace - line.size();
		const StringVec lines = appbuild::SplitString(Descriptions[n],"\n");
		for(auto line : lines)
		{
			std::cout << std::string(space,' ') << line << std::endl;
			space = DescMaxSpace + 2;// For subsequent lines.
		}
	}
}

void CommandLineOptions::PrintVersion()const
{
#ifdef BUILT_BY_APPBUILD
	std::cout << "Build date and time " << APP_BUILD_DATE_TIME << std::endl;
	std::cout << "Build date " << APP_BUILD_DATE << std::endl;
	std::cout << "Build time " << APP_BUILD_TIME << std::endl;
#endif
}


void CommandLineOptions::PrintGetHelp()const
{
	std::cout << "Try 'appbuild --help' for more information."  << std::endl;
}
