#include <sys/stat.h>
#include <limits.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "json.h"

#include "new_project.h"
#include "misc.h"
#include "dependencies.h"
#include "source_files.h"
#include "configuration.h"
#include "project.h"
#include "logging.h"

typedef struct stat FileStats;

namespace appbuild{
//////////////////////////////////////////////////////////////////////////
int CreateNewProject(const std::string& pNewProjectName,int pLoggingMode)
{
    // Check name does not use any pathing characters.
    if( pNewProjectName.find('/') != std::string::npos )
    {
        std::cout << "--new-project arg contained the disallowed character '/'" << std::endl;
        return EXIT_FAILURE;
    }

    // Name good, now make sure folder does not already exsist.
    const std::string projectPath = "./" + pNewProjectName;
    const std::string pathedProjectFilename = projectPath + "/" + pNewProjectName + ".proj";

    // Make sure we're not going to overwrite something that is already there.
    if( DirectoryExists(pathedProjectFilename) )
    {
        std::cout << "New project already exsists, can not make new project. [" << projectPath << "]" << std::endl;
        return EXIT_FAILURE;
    }

    SourceFiles SourceFiles(GetPath(pathedProjectFilename));

    if( DirectoryExists(projectPath) )
    {
        // Folder already here, so look for source files to add.

    }
    else
    {
        if( MakeDir(projectPath) == false )
        {
            std::cout << "Failed to create path for the new project to reside in, [" << projectPath << "]" << std::endl;
            return EXIT_FAILURE;
        }

        // Going to be making a new folder, so add the folder and a new empty source file.
        const std::string defaultMainName = pNewProjectName + ".cpp";
        const std::string pathedSourceFilename = projectPath + "/" + defaultMainName;

        std::ofstream sourceFile(pathedSourceFilename);
        if( sourceFile.is_open() )
        {
            // Now write a hello world....

            sourceFile << "#include <iostream>" << std::endl;
            sourceFile << std::endl;
            sourceFile << "int main(int argc, char *argv[])" << std::endl;
            sourceFile << "{" << std::endl;
            sourceFile << "// Say hello to the world!" << std::endl;
            sourceFile << "    std::cout << \"Hello world, an app compiled with appbuild.\" << std::endl;" << std::endl;
            sourceFile << std::endl;
            sourceFile << "// Display the constants defined by app build. " << std::endl;
            sourceFile << "    std::cout << \"Build date and time \" << APP_BUILD_DATE_TIME << std::endl;" << std::endl;
            sourceFile << "    std::cout << \"Build date \" << APP_BUILD_DATE << std::endl;" << std::endl;
            sourceFile << "    std::cout << \"Build time \" << APP_BUILD_TIME << std::endl;" << std::endl;
            sourceFile << std::endl;
            sourceFile << "// And quit" << std::endl;
            sourceFile << "    return 0;" << std::endl;
            sourceFile << "}" << std::endl;
            sourceFile << std::endl;

            sourceFile.close();

            // Now the file has been written we can add it. Would fail if done before.
            if( SourceFiles.AddFile(defaultMainName) == false )
            {
                std::cout << "Failed to add default cpp file to the project, [" << defaultMainName << "]" << std::endl;
                return EXIT_FAILURE;
            }

        }
        else
        {
            std::cout << "Failed to create default source file for new project, [" << pathedSourceFilename << "]" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "SourceFiles.size() == " << SourceFiles.size() << std::endl;

    // Now build the project object.
    Project newProject(pathedProjectFilename,SourceFiles,true,pLoggingMode);
    if( newProject == false )
    {
        std::cout << "Failed to create default project file, [" << pathedProjectFilename << "]" << std::endl;        
        return EXIT_FAILURE;
    }

    rapidjson::Document jsonOutput;
    jsonOutput.SetObject(); // Root object is an object not an array.
    newProject.Write(jsonOutput);
    if( appbuild::SaveJson(pathedProjectFilename,jsonOutput) )
    {
        std::cout << "New project file saved" << std::endl;
    }
    else
    {
        std::cout << "Failed to write to the destination file, [" << pathedProjectFilename << "]" << std::endl;
    }

    return EXIT_SUCCESS;    
}

//////////////////////////////////////////////////////////////////////////
};//namespace appbuild
