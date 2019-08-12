#include <sys/stat.h>
#include <limits.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "she_bang.h"
#include "misc.h"
#include "string_types.h"
#include "dependencies.h"
#include "json_writer.h"
#include "source_files.h"
#include "configuration.h"
#include "project.h"
#include "logging.h"

typedef struct stat FileStats;

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
static bool CompareFileTimes(const std::string& pSourceFile,const std::string& pDestFile)
{
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

    // Now we need to create the path to the compiled exec.
    // This is done so we only have to build when something changes.
    // To ensure no clashes I take the fulled pathed source file name
    // and add /tmp to the start for the temp folder. I also add .exe at the end.
    const std::string exename = std::string("/tmp") + sourcePathedFile + ".exe";

    // Also create the project file that will be used to build the exe if needs be.
    const std::string project = std::string("/tmp") + sourcePathedFile + ".proj";

    // We need the source file without the shebang too.
    const std::string tempSourcefile = std::string("/tmp") + sourcePathedFile;

    // The temp folder that it's all done in.
    const std::string projectTempFolder = GetPath(project);

#ifdef DEBUG_BUILD
    std::cout << "CWD " << CWD << std::endl;
    std::cout << "sourcePathedFile " << sourcePathedFile << std::endl;
    std::cout << "exenameFileExists " << exename << std::endl;
#endif

    // Make sure our temp folder is there.
    MakeDir(projectTempFolder);

    StringVec libraryFiles;
    libraryFiles.push_back("stdc++");
    libraryFiles.push_back("pthread");

    // First see if the source file that does not have the shebang in it is there.
    bool rebuildNeeded = CompareFileTimes(sourcePathedFile,tempSourcefile);
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
#ifdef DEBUG_BUILD
                std::cout << line << std::endl;
#endif
                if( line[0] == '#' && line[1] == '!' ) // Is it the shebang? If so remove it.
                {
                    foundShebang = true;
                }
                else if( CompareNoCase(line.c_str(),"#APPBUILD_LIBS ",15) ) // Is it a pragma to add a libs?
                {
                    // First entry will be the #APPBUILD_LIBS, so skip that bit.
                    const std::string libs = line.substr(15);
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

    // What I am doing is creating a project file, and then calling the normal code path for creating an execuatable.
    if( FileExists(project) == false || rebuildNeeded )
    {
#ifdef DEBUG_BUILD
        std::cout << "Project file rebuild needed!" << std::endl;
#endif

#ifdef DEBUG_BUILD
    std::cout << "Linking with libs: ";
    for(const auto& lib : libraryFiles)
        std::cout << lib << " ";

    std::cout << std::endl;
#endif

        rebuildNeeded = true;
        SourceFiles sourceFiles(CWD);
        BuildConfigurations buildConfigurations;

        JsonWriter jsonOutput;
        jsonOutput.StartObject();
            jsonOutput.StartObject("configurations");
                // Setup as a release build
                jsonOutput.StartObject("shebang");
                    jsonOutput.AddObjectItem("default",true);
                    jsonOutput.AddObjectItem("output_path","./");
                    jsonOutput.AddObjectItem("output_name",GetFileName(exename));
                    jsonOutput.AddObjectItem("optimisation","2");
                    jsonOutput.AddObjectItem("debug_level","0");

                    jsonOutput.StartArray("libs");
                    for(const auto& lib : libraryFiles)
                        jsonOutput.AddArrayItem(lib);
                    jsonOutput.EndArray();

                jsonOutput.EndObject();
            jsonOutput.EndObject();
            jsonOutput.StartArray("source_files");
                jsonOutput.AddArrayItem(GetFileName(sourcePathedFile));
            jsonOutput.EndArray();
        jsonOutput.EndObject();

#ifdef DEBUG_BUILD
    std::cout << jsonOutput.Get() << std::endl;
#endif

        // We need to make the project file.
        std::ofstream updatedProjectFile(project);
        if( updatedProjectFile )
        {
            updatedProjectFile << jsonOutput.Get();
        }
        else
        {
            std::cerr << "Failed to write to the tempory project file [" << project << "] needed to create the cached exe, please check there is a system temp folder that can be used." << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Now do the normal build thing.
    const std::string file = GetFileName(project);

    if( chdir(projectTempFolder.c_str()) != 0 )
    {
        std::cerr << "Failed to " << projectTempFolder << std::endl;
        return EXIT_FAILURE;
    }

    // No point doing the link stage is the source file has not changed!
    if( rebuildNeeded )
    {
#ifdef DEBUG_BUILD
        int loggingMode = LOG_VERBOSE;
#else
        int loggingMode = LOG_ERROR;
#endif

        Project TheProject(file,1,loggingMode,false,0);

        if( TheProject == false )
        {
            std::cerr << "There was an error in the project file \'" << file << "\'" << std::endl;
            return EXIT_FAILURE;
        }

        const appbuild::Configuration* ActiveConfig = TheProject.GetActiveConfiguration("");
        if( ActiveConfig == NULL )
        {
            std::cerr << "No configuration found in the project file \'" << file << "\'" << std::endl;
            return EXIT_FAILURE;
        }

        if( ActiveConfig->GetOk() == false )
        {
            std::cerr << "There is a problem with the configuration \'" << ActiveConfig->GetName() << "\' in the project file \'" << file << "\'. Please check the output for errors." << std::endl;
            return EXIT_FAILURE;
        }

        if( TheProject.Build(ActiveConfig) == false )
        {
            std::cerr << "Build failed for project file \'" << file << "\'" << std::endl;
            return EXIT_FAILURE;
        }
    }


    // See if we have the output file, if so run it!
    if( FileExists(exename) )
    {// I will not be using ExecuteShellCommand as I need to replace this exec to allow the input and output to be taken over.

        if( chdir(CWD.c_str()) != 0 )
        {
            std::cerr << "Failed to return to the orginal run folder " << CWD << std::endl;
            return EXIT_FAILURE;
        }

#ifdef DEBUG_BUILD
std::cout << "Running exec...." << std::endl;
#endif

        char** TheArgs = new char*[(argc - 3) + 2];// -3 for the args sent into appbuild shebang, then +1 for the NULL and +1 for the file name as per convention, see https://linux.die.net/man/3/execlp.
        int c = 0;
        TheArgs[c++] = CopyString(exename);
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
        std::cerr << "Failed to find executable " << exename << std::endl;
        return EXIT_FAILURE;
    }

    if( chdir(CWD.c_str()) != 0 )
    {
        std::cerr << "Failed to return to the orginal run folder " << CWD << std::endl;
    }

    return EXIT_FAILURE;
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
