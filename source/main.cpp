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
#include <string>
#include <vector>

#include "json.h"
#include "string_types.h"
#include "project.h"
#include "misc.h"
#include "she_bang.h"
#include "new_project.h"
#include "logging.h"

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

	CommandLineOptions Args(argc,argv);

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
		// Options read ok, now parse the project.
		for(const std::string& file : Args.GetProjectFiles() )
		{
			// Read the project file.
			rapidjson::Document ProjectJson;

			if( appbuild::ReadJson(file,ProjectJson) )
			{
				// Validate the json.
				if( appbuild::ValidateJsonAgainstSchema(ProjectJson) == false )
				{
					return EXIT_FAILURE;
				}

				rapidjson::Value& projectRoot = ProjectJson;
				// Is the project settings embedded in another file?
				if( Args.GetProjectIsEmbedded() )
				{
					if( projectRoot.HasMember(Args.GetProjectJsonKey()) )
					{
						projectRoot = projectRoot[Args.GetProjectJsonKey()];
					}
					else
					{
						std::cout << "The project was not found in Json object \'" << Args.GetProjectJsonKey() << "\' inside the file \'" << file << "\'" << std::endl;
						Args.PrintGetHelp();
						return EXIT_FAILURE;
					}
					
				}

				const std::string projectPath = appbuild::GetPath(file);
				appbuild::Project TheProject(projectRoot,file,projectPath,Args.GetNumThreads(),Args.GetLoggingMode(),Args.GetReBuild(),Args.GetTruncateOutput());
				if( TheProject )
				{
					TheProject.AddGenericFileDependency(file);
					if( Args.GetInteractiveMode() )
					{
						if( Args.ProcessInteractiveMode(TheProject) == false )
						{
							// User aborted interactive mode, so return.
							return EXIT_FAILURE;
						}
					}

					const std::string configname = Args.GetActiveConfig(TheProject.FindDefaultConfigurationName());

					if( Args.GetUpdatedProject() )
					{
						std::cout << "Updating project file and writing the results too \'" << Args.GetUpdatedOutputFileName() << "\'" << std::endl;

						rapidjson::Document jsonOutput;
						jsonOutput.SetObject(); // Root object is an object not an array.
						TheProject.Write(jsonOutput);
						if( appbuild::SaveJson(Args.GetUpdatedOutputFileName(),jsonOutput) )
						{
							std::cout << "Project file updated" << std::endl;
						}
						else
						{
							std::cout << "Failed to write to the destination file." << std::endl;
						}
					}// This is an if else as after loading, if the project is to be updated with the defaults, then we do not want to then also build.
					else if( configname.size() > 0 )
					{
						const std::chrono::system_clock::time_point build_start = std::chrono::system_clock::now();
						if( TheProject.Build(configname) )
						{
							if( Args.GetTimeBuild() )
							{
								std::cout << "Build took: " << appbuild::GetTimeDifference(build_start,std::chrono::system_clock::now()) << std::endl;
							}

							if( Args.GetRunAfterBuild() )
							{
								TheProject.RunOutputFile(configname);
							}
						}
						else
						{
							std::cout << "Build failed for project file \'" << file << "\'" << std::endl;
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
			else
			{
				std::cout << "The project file \'" << file << "\' could not be loaded" << std::endl;
			}
		}
	}
	else
	{
		Args.PrintGetHelp();
	}

	return EXIT_SUCCESS;
}


#define ARGUMENTS																																									\
		DEF_ARG(ARG_HELP,no_argument,							'h',"help","Display this help and exit.")																			\
		DEF_ARG(ARG_VERSION,no_argument,						'v',"version","Output version information and exit.")																\
		DEF_ARG(ARG_SHOW_TYPE_SIZES,no_argument,				's',"type-info","Output the byte sizes of a selection of types at the time appbuild was built.")					\
		DEF_ARG(ARG_VERBOSE,no_argument,						'V',"verbose","Print more information about progress.")																\
		DEF_ARG(ARG_QUIET,no_argument,				    		'q',"quiet","Print no information about progress, just errors will be displayed.")                       			\
		DEF_ARG(ARG_REBUILD,no_argument,						'r',"rebuild","Clean and rebuild all the source files.")															\
		DEF_ARG(ARG_RUN_AFTER_BUILD,no_argument,				'x',"run-after-build","If the build is successful then eXecute the app, but only if one project file submitted.")	\
		DEF_ARG(ARG_NUM_THREADS,required_argument,				'n',"num-threads","Sets the number of threads to use when tasks can be done in parallel.")							\
		DEF_ARG(ARG_ACTIVE_CONFIG,required_argument,			'c',"active-config","Builds the given configuration, if found.")													\
		DEF_ARG(ARG_UPDATE_PROJECT,required_argument,			'u',"update-project","Reads in the project file passed in then writes out an updated version with all the default paramiters\nfilled in that were not in the source.\nProject is not built if this option is specified.")	\
		DEF_ARG(ARG_TRUNCATE_OUTPUT,required_argument,			't',"truncate-output","Truncates the output to the first N lines, if you're getting too many errors this can help.")	\
		DEF_ARG(ARG_TIME_BUILD,no_argument,						'T',"time-build","Shows the total time of the build from start to finish.")												\
		DEF_ARG(ARG_SHEBANG,no_argument,						'#',"she-bang","Makes the c/c++ file with appbuild defined as a shebang run as if it was an executable. JIT Compiled.") \
		DEF_ARG(ARG_NEW_PROJECT,required_argument,				'P',"new-project","Where arg is the new project name, makes a folder in the current working directory of the passed name with a simple hello world cpp file\nand a default project file with release and debug configurations.\nIf the folder already exists searches folder for source files and adds them to a new project file.\nIf a project file already exists then it will fail.") \
		DEF_ARG(ARG_INTERACTIVE,no_argument,					'i',"interactive","If the project has multiple configurations then a menu will allow you to select which to build.\nThe default configuration, if marked, will be selected by default.")	\
		DEF_ARG(ARG_DISPLAY_PROJECT_SCHEMA,optional_argument,	'S',"show-schema","Displays the schema of the project json. Arg is an optional filename, if not given then will output to std::cout.\nIf arg is specfied will save the schema to the file.")						\
		DEF_ARG(ARG_EMBEDDED_PROJECT,required_argument,			'e',"embedded-project","A project can be in the root of other json data, that is embedded in the json file of another application.\nThis allows you to state the key name in the root document where to find the appbuild proect.") \


enum eArguments
{
#define DEF_ARG(ARG_NAME,TAKES_ARGUMENT,ARG_SHORT_NAME,ARG_LONG_NAME,ARG_DESC)	ARG_NAME = ARG_SHORT_NAME,
	ARGUMENTS
#undef DEF_ARG
};

CommandLineOptions::CommandLineOptions(int argc, char *argv[]):
	mShowHelp(false),
	mShowVersion(false),
	mShowTypeSizes(false),
	mRunAfterBuild(false),
	mReBuild(false),
	mTimeBuild(false),
	mInteractiveMode(false),
	mDisplayProjectSchema(false),
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

		case ARG_SHOW_TYPE_SIZES:
			mShowTypeSizes = true;
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

		case ARG_INTERACTIVE:
			mInteractiveMode = true;
			break;

		case ARG_DISPLAY_PROJECT_SCHEMA:
			mDisplayProjectSchema = true;
			if( optarg )
			{
				mSchemaSaveFilename = optarg;
			}
			break;

		case ARG_EMBEDDED_PROJECT:
			if( optarg )
				mProjectJsonKey = optarg;
			else
			{
				mShowHelp = true;
				std::cout << "Option -e (embedded-project) was not passed correct value. -e keyname or -embedded-project=keyname where keyname is the object key in the root document." << std::endl;
			}
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
    		std::cout << "-# (she-bang) must be the first and only option, it can not be used with other options." << std::endl;
            mShowHelp = true;
			break;

		case ARG_NEW_PROJECT:
			if( optarg && strlen(optarg) > 0 )
			{
				mNewProjectName = optarg;
			}
			else
			{
	    		std::cout << "--new-project was not passed a valid project name." << std::endl;
				mShowHelp = true;
			}
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
		// If only one is found use that, else don't use any as it maybe not what they intended.
		const appbuild::StringVec ProjFiles = appbuild::FindFiles(appbuild::GetCurrentWorkingDirectory(),"*.proj");
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

	// I build three vectors of strings for all options. So I can measure and then line up the text with spaces. Makes life easier when adding new options.
	appbuild::StringVec ShortArgs;
	appbuild::StringVec LongArgs;
	appbuild::StringVec Descriptions;

#define DEF_ARG(ARG_NAME,TAKES_ARGUMENT,ARG_SHORT_NAME,ARG_LONG_NAME,ARG_DESC)	\
	if(true)																	\
	{																			\
		ShortArgs.push_back(#ARG_SHORT_NAME);									\
		if(TAKES_ARGUMENT==required_argument)									\
		{																		\
			LongArgs.push_back("--" ARG_LONG_NAME "=arg");						\
		}																		\
		else if(TAKES_ARGUMENT==optional_argument)								\
		{																		\
			LongArgs.push_back("--" ARG_LONG_NAME "[=arg]");					\
		}																		\
		else																	\
		{																		\
			LongArgs.push_back("--" ARG_LONG_NAME);								\
		}																		\
		Descriptions.push_back(ARG_DESC);										\
	};

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
		const appbuild::StringVec lines = appbuild::SplitString(Descriptions[n],"\n");
		for(auto line : lines)
		{
			std::cout << std::string(space,' ') << line << std::endl;
			space = DescMaxSpace + 2;// For subsequent lines.
		}
	}
}

void CommandLineOptions::PrintVersion()const
{
#ifdef APP_BUILD_VERSION
	std::cout << "Version " << APP_BUILD_VERSION << std::endl;
#endif

#ifdef APP_BUILD_DATE
	std::cout << "Build date and time " << APP_BUILD_DATE << std::endl;
#endif

#ifdef APP_BUILD_TIME
	std::cout << "Build time " << APP_BUILD_TIME << std::endl;
#endif

#ifdef APP_BUILD_GIT_BRANCH
	// APP_BUILD_GIT_BRANCH maybe zero length if git is a bit old, as in RPi images.
	// So need to check that.
	if( std::string(APP_BUILD_GIT_BRANCH).size() > 1 )
	{
		std::cout << "Build from git branch " << APP_BUILD_GIT_BRANCH << std::endl;
	}
#endif
}

void CommandLineOptions::PrintTypeSizes()const
{
	std::cout  << std::endl << "***** Type size info at build time *****" << std::endl;
#define SHOW_TYPE_SIZE(__TYPE__) std::cout << "sizeof(" << #__TYPE__ << ") = " << sizeof(__TYPE__)  << std::endl;
	SHOW_TYPE_SIZE(void*);
	SHOW_TYPE_SIZE(char*);
	SHOW_TYPE_SIZE(int*);
	std::cout << std::endl;
	SHOW_TYPE_SIZE(char);
	SHOW_TYPE_SIZE(short);
	SHOW_TYPE_SIZE(int);
	SHOW_TYPE_SIZE(long);
	SHOW_TYPE_SIZE(long int);
	SHOW_TYPE_SIZE(long long);
	SHOW_TYPE_SIZE(uint8_t);
	SHOW_TYPE_SIZE(uint16_t);
	SHOW_TYPE_SIZE(uint32_t);
	SHOW_TYPE_SIZE(uint64_t);
	SHOW_TYPE_SIZE(float);
	SHOW_TYPE_SIZE(double);
	SHOW_TYPE_SIZE(size_t);
	SHOW_TYPE_SIZE(timeval);
	SHOW_TYPE_SIZE(__time_t);
	SHOW_TYPE_SIZE(__suseconds_t);
	std::cout << std::endl;
	SHOW_TYPE_SIZE(std::string);
	SHOW_TYPE_SIZE(std::vector<int>);
	SHOW_TYPE_SIZE(std::vector<uint32_t>);
	SHOW_TYPE_SIZE(std::vector<long>);
#undef SHOW_TYPE_SIZE
	std::cout << "*********************************"  << std::endl << std::endl;
}

void CommandLineOptions::PrintGetHelp()const
{
	std::cout << "Try 'appbuild --help' for more information."  << std::endl;
}

void CommandLineOptions::PrintProjectSchema()const
{
	const std::string& schema = appbuild::GetProjectSchema();

	if( GetWriteSchemaToFile() )
	{
		const std::string& schemaFilename = GetSchemaSaveFilename();
		std::ofstream file(schemaFilename);
		if( file.is_open() )
		{
			file << schema << std::endl;
			std::cout << "Schema saved too  " << schemaFilename << std::endl;
		}
		else
		{
			std::cout << "Failed to open file to write the project schema too,  " << schemaFilename << std::endl;
		}
	}
	else
	{
		std::cout << schema << std::endl;
	}
}

bool CommandLineOptions::ProcessInteractiveMode(appbuild::Project& pTheProject)
{
	std::cout << "Choose the configuration to build or just enter for the default." << std::endl;
	const appbuild::StringVec Configs = pTheProject.GetConfigurationNames();
	const std::string defaultName = pTheProject.FindDefaultConfigurationName();
	
	int index = 0;
	for( auto c : Configs )
	{
		index++;
		std::cout << c << " [" << index << "]";

		if( c == defaultName )
		{
			std::cout << "*";
		}

		std::cout << std::endl;
	}
	std::cout << "Choose [1-" << index << "] ";
		
	std::cout << std::flush;

	int choice = std::cin.get();

	if( choice == '\n' )
	{
		std::cout << "Using default, " << defaultName << std::endl;
		mActiveConfig = defaultName;
	}
	else
	{
		choice -= '1';
		if( choice >= 1 && choice <= index  )
		{
			std::cout << Configs[choice] << " chosen" << std::endl;
			mActiveConfig = Configs[choice];
		}
		else
		{// Assume they chose to quit.
			return false;
		}
	}

	if( mActiveConfig.size() > 0 )
	{
		std::cout << "appbuild -c " << mActiveConfig << std::endl;
	}

	return true;
}
