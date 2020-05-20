#include <sys/stat.h>
#include <limits.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>

#include "json.h"

#include "she_bang.h"
#include "misc.h"
#include "dependencies.h"
#include "source_files.h"
#include "configuration.h"
#include "project.h"
#include "logging.h"

typedef struct stat FileStats;

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
#ifdef DEBUG_BUILD
    const int LOGGING_MODE = LOG_VERBOSE;
#else
    const int LOGGING_MODE = LOG_ERROR;
#endif


/**
 * @brief Checks if the file for source is newer than of dest.
 * Also will pretend that it is newer if any of the stats fail.
 * This is a bespoke to my needs here, should not be exported to rest of the code.
 * 
 * @param pSourceFile 
 * @param pDestFile 
 * @return true 
 * @return false 
 */
static bool CompareFileTimes(const std::string& pSourceFile,const std::string& pDestFile)
{
    if( LOGGING_MODE == LOG_VERBOSE )
    {
        std::cout << "CompareFileTimes(" << pSourceFile << "," << pDestFile << ")" << std::endl;
    }

    FileStats Stats;
    if( stat(pDestFile.c_str(), &Stats) == 0 && S_ISREG(Stats.st_mode) )
    {
        timespec dstFileTime = Stats.st_mtim;

        if( stat(pSourceFile.c_str(), &Stats) == 0 && S_ISREG(Stats.st_mode) )
        {
            timespec srcFileTime = Stats.st_mtim;

            if(srcFileTime.tv_sec == dstFileTime.tv_sec)
                return srcFileTime.tv_nsec > dstFileTime.tv_nsec;

            return srcFileTime.tv_sec > dstFileTime.tv_sec;
        }
    }

    return true;
}

int BuildFromShebang(int argc,char *argv[])
{
    if( argv[2] == NULL )
        return EXIT_FAILURE;

    const size_t ARG_MAX = 4096;

    const std::string CWD = GetCurrentWorkingDirectory() + "/";
    // Get the source file name, if not return an error.
    // Should be in argv[2]
    const std::string sourcePathedFile = CleanPath(CWD + argv[2]);

    // Sanity check, is file there?
    if( FileExists(sourcePathedFile) == false )
        return EXIT_FAILURE;

    // Get the path to the source file, need this as we need to insert the project configuration name so we can find it.
    const std::string sourceFilePath = GetPath(sourcePathedFile);

    // The projects configuration name we'll use.
    const std::string configName = "shebang";

    // This is the temp folder path we use to cache build results.
    const std::string tempFolderPath("/tmp/appbuild/");

    // Use the source file as the 'project name' for error reports.
    // This is the name of the project that the user will expect to see in errors.
    const std::string usersProjectName = sourcePathedFile;

    // Now we need to create the path to the compiled exec.
    // This is done so we only have to build when something changes.
    // To ensure no clashes I take the fully pathed source file name
    // and add /tmp to the start for the temp folder. I also add .exe at the end.
    const std::string exename = GetFileName(sourcePathedFile) + ".exe";
    const std::string pathedExeName = CleanPath(tempFolderPath + sourceFilePath + "/bin/" + configName + "/" + exename);

    // We need the source file without the shebang too.
    const std::string tempSourcefile = CleanPath(tempFolderPath + sourcePathedFile);

    // Also create the project file that will be used to build the exe if needs be.
    // We don't really make this file but the objects involved need to think they were loaded from a file. Maybe that is an error in the design.
    const std::string fakeProjectFilename = tempSourcefile + ".proj";


    // The temp folder that it's all done in.
    const std::string projectTempFolder = GetPath(tempSourcefile);

#ifdef DEBUG_BUILD
    std::cout << "CWD " << CWD << std::endl;
    std::cout << "sourcePathedFile " << sourcePathedFile << std::endl;
    std::cout << "exenameFileExists " << pathedExeName << std::endl;
#endif

    // Make sure our temp folder is there.
    MakeDir(projectTempFolder);

    StringVec libraryFiles;

    // First see if the source file that does not have the shebang in it is there.
    bool rebuildNeeded = CompareFileTimes(sourcePathedFile,tempSourcefile);

    // Now, it may have built but failed to link, so we then need to change is the output is there.
    if( rebuildNeeded == false )
    {
        rebuildNeeded = CompareFileTimes(sourcePathedFile,pathedExeName);
    }

    if( rebuildNeeded )
    {
#ifdef DEBUG_BUILD
        std::cout << "Source file rebuild needed!" << std::endl;
#endif
        // Ok, we better build it.
        // Copy all lines of the file over excluding the first line that has the shebang.
        std::string line;
        std::ifstream oldSource(sourcePathedFile);
        bool foundShebang = false;

        std::ofstream newSource(tempSourcefile);
        if( newSource )
        {
            while( std::getline(oldSource,line) )
            {
                if( LOGGING_MODE == LOG_VERBOSE )
                {
                    std::cout << line << std::endl;
                }

                if( line[0] == '#' && line[1] == '!' ) // Is it the shebang? If so remove it.
                {
                    foundShebang = true;
                }
                else if( CompareNoCase(line.c_str(),"#APPBUILD_LIBS ",15) ) // Is it a pragma to add a libs?
                {
                    // First entry will be the #APPBUILD_LIBS, so skip that bit.
                    const std::string libs = line.substr(15);

                    if( LOGGING_MODE == LOG_VERBOSE )
                    {
                        std::cout << "APPBUILD_LIBS entry found, libs are " << libs;
                    }

                    if( libs.size() > 0 )
                    {
                        for (size_t p = 0, q = 0; p != libs.npos; p = q)
                        {
                            libraryFiles.push_back(libs.substr(p + (p != 0), (q = libs.find(" ", p + 1)) - p - (p != 0)));
                        }
                    }
                }
                else
                {
                    newSource << line << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "Failed to parse the source file into new temp file..." << std::endl;
            return EXIT_FAILURE;
        }

        if( !foundShebang )
        {
            std::cerr << "Failed to parse the source file..." << std::endl;
            return EXIT_FAILURE;
        }
    }
    else if( LOGGING_MODE == LOG_VERBOSE )
    {
        std::cout << "Skipping rebuild, executable is not out of date." << std::endl;
    }

    // Now do the normal build thing.
    const std::string file = GetFileName(fakeProjectFilename);

    if( chdir(projectTempFolder.c_str()) != 0 )
    {
        std::cerr << "Failed to " << projectTempFolder << std::endl;
        return EXIT_FAILURE;
    }

    // No point doing the link stage is the source file has not changed!
    if( rebuildNeeded )
    {
        SourceFiles SourceFiles(CWD,LOGGING_MODE);
        SourceFiles.AddFile(GetFileName(sourcePathedFile));

        std::unique_ptr<Configuration> newConfig = std::make_unique<Configuration>(configName,exename,projectTempFolder,LOGGING_MODE,true,"2","0");
        newConfig->AddDefine("NDEBUG");
        newConfig->AddDefine("RELEASE_BUILD");

        for( auto lib : libraryFiles )
        {
            newConfig->AddLibrary(lib);
        }

        ConfigurationsVec configs;
        configs.push_back(std::move(newConfig));

        Project sheBangProject(fakeProjectFilename,SourceFiles,configs,LOGGING_MODE);
        if( sheBangProject == false )
        {
            std::cout << "Failed to create default project, [" << usersProjectName << "]" << std::endl;        
            return EXIT_FAILURE;
        }

        const std::string configName = sheBangProject.FindDefaultConfigurationName();
        if( configName.size() == 0 )
        {
            std::cerr << "No configuration found in the shebang virtual project, implementation error, please log a bug. \'" << file << "\'" << std::endl;
            return EXIT_FAILURE;
        }

        if( sheBangProject.Build(configName) == false )
        {
            std::cerr << "Build failed for project file \'" << file << "\'" << std::endl;
            return EXIT_FAILURE;
        }
    }


    // See if we have the output file, if so run it!
    if( FileExists(pathedExeName) )
    {// I will not be using ExecuteShellCommand as I need to replace this exec to allow the input and output to be taken over.

        if( chdir(CWD.c_str()) != 0 )
        {
            std::cerr << "Failed to return to the original run folder " << CWD << std::endl;
            return EXIT_FAILURE;
        }

#ifdef DEBUG_BUILD
std::cout << "Running exec...." << std::endl;
#endif

        char** TheArgs = new char*[(argc - 3) + 2];// -3 for the args sent into appbuild shebang, then +1 for the NULL and +1 for the file name as per convention, see https://linux.die.net/man/3/execlp.
        int c = 0;
        TheArgs[c++] = CopyString(pathedExeName);
        for( int n = 3 ; n < argc ; n++ )
        {
            char* str = CopyString(argv[n],ARG_MAX);
            if(str)
            {
                //Trim leading white space.
                while(isspace(str[0]) && str[0])
                    str++;
                if(str[0])
                    TheArgs[c++] = str;
            }
        }
        TheArgs[c++] = NULL;

#ifdef DEBUG_BUILD
for(int n = 0 ; TheArgs[n] != NULL ; n++ )
std::cout << "TheArgs[n] " << TheArgs[n] << std::endl;
#endif

        // This replaces the current process so no need to clean up the memory leaks before here. ;)
        execvp(TheArgs[0], TheArgs);

        std::cerr << "ExecuteShellCommand execl() failure!" << std::endl << "This print is after execl() and should not have been executed if execl were successful!" << std::endl;
        _exit(1);

    }
    else
    {
        std::cerr << "Failed to find executable " << pathedExeName << std::endl;
        return EXIT_FAILURE;
    }

    if( chdir(CWD.c_str()) != 0 )
    {
        std::cerr << "Failed to return to the original run folder " << CWD << std::endl;
    }

    return EXIT_FAILURE;
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
