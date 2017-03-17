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
#include <unistd.h>
#include <getopt.h>

#include "project.h"
#include "misc.h"

class CommandLineOptions
{
public:
	CommandLineOptions(int argc, char *argv[]);

	bool GetShowHelp()const{return mShowHelp;}
	bool GetShowVersion()const{return mShowVersion;}
	bool GetVerboseOutput()const{return mVerboseOutput;}
	bool GetRunAfterBuild()const{return mRunAfterBuild || mSudoRunAfterBuild;}
	bool GetSudoRunAfterBuild()const{return mSudoRunAfterBuild;}
	bool GetReBuild()const{return mReBuild;}
	int  GetNumThreads()const{return mNumThreads;}
	std::vector<std::string> GetProjectFiles()const{return mProjectFiles;}
	const std::string& GetActiveConfig()const{return mActiveConfig;}

	void PrintHelp()const;
	void PrintVersion()const;
	void PrintGetHelp()const;

private:
	bool mShowHelp;
	bool mShowVersion;
	bool mVerboseOutput;
	bool mRunAfterBuild;
	bool mSudoRunAfterBuild;
	bool mReBuild;
	int  mNumThreads;
	std::vector<std::string> mProjectFiles;
	std::string mActiveConfig;
};


int main(int argc, char *argv[])
{
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
			appbuild::Project TheProject(file,Args.GetNumThreads(),Args.GetVerboseOutput(),Args.GetReBuild());
			if( TheProject )
			{
				const std::string configname = Args.GetActiveConfig();
				const appbuild::Configuration* ActiveConfig = TheProject.FindConfiguration(configname);
				if( ActiveConfig )
				{
					if( TheProject.Build(ActiveConfig) && Args.GetRunAfterBuild() )
					{
						TheProject.RunOutputFile(ActiveConfig,Args.GetSudoRunAfterBuild());
					}
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

	return 0;
}


#define ARGUMENTS																																											\
		DEF_ARG(ARG_NUM_THREADS,required_argument,		'n',"num-threads",    "           Sets the number of threads to use when tasks can be done in parallel.")							\
		DEF_ARG(ARG_HELP,no_argument,					'h',"help","                      Display this help and exit.")																		\
		DEF_ARG(ARG_VERSION,no_argument,				'v',"version","                   Output version information and exit.")															\
		DEF_ARG(ARG_VERBOSE,no_argument,				'V',"verbose","                   Print more information about progress.")															\
		DEF_ARG(ARG_RUN_AFTER_BUILD,no_argument,		'r',"run-after-build","           If the build is successful then run the app, but only if one project file submitted.")			\
		DEF_ARG(ARG_SUDO_RUN_AFTER_BUILD,no_argument,	'R',"sudo-run-after-build","      If the build is successful then run the app as SUDO, but only if one project file submitted.")	\
		DEF_ARG(ARG_REBUILD,no_argument,				'B',"rebuild","                   Clean and rebuild all the source files.")															\
		DEF_ARG(ARG_ACTIVE_CONFIG,required_argument,	'c',"active-config","             Builds the given configuration, if found.")															\


enum eArguments
{
#define DEF_ARG(ARG_NAME,TAKES_ARGUMENT,ARG_SHORT_NAME,ARG_LONG_NAME,ARG_DESC)	ARG_NAME = ARG_SHORT_NAME,
	ARGUMENTS
#undef DEF_ARG
};

CommandLineOptions::CommandLineOptions(int argc, char *argv[]):
	mShowHelp(false),
	mShowVersion(false),
	mVerboseOutput(false),
	mRunAfterBuild(false),
	mSudoRunAfterBuild(false),
	mReBuild(false),
	mNumThreads(std::thread::hardware_concurrency())
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
			mVerboseOutput = true;
			break;

		case ARG_RUN_AFTER_BUILD:
			mRunAfterBuild = true;
			break;

		case ARG_SUDO_RUN_AFTER_BUILD:
			mSudoRunAfterBuild = true;
			break;

		case ARG_REBUILD:
			mReBuild = true;
			break;

		case ARG_ACTIVE_CONFIG:
			if( optarg )
				mActiveConfig = optarg;
			else
				std::cout << "Option -c (active-config) was not passed correct value. -c release or --active-config=release" << std::endl;
			break;

		default:

			break;
		}
	}

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
	std::cout << "Mandatory arguments to long options are mandatory for short options too." << std::endl;

#define DEF_ARG(ARG_NAME,TAKES_ARGUMENT,ARG_SHORT_NAME,ARG_LONG_NAME,ARG_DESC) std::cout << "  -" << ARG_SHORT_NAME << ", --" << ARG_LONG_NAME << (TAKES_ARGUMENT==required_argument?"=arg":"") << ARG_DESC << std::endl;
	ARGUMENTS
#undef DEF_ARG
}

void CommandLineOptions::PrintVersion()const
{

}


void CommandLineOptions::PrintGetHelp()const
{
	std::cout << "Try 'appbuild --help' for more information."  << std::endl;
}


