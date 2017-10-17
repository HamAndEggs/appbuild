********
Appbuild
********
Appbuild is a build tool for creating libraries and executables written in c/c++.

The aim is to have a simple single file to represent a c/c++ project that is easy to maintain and has all the features that you would expect without having to learn scripting language.

**Features**
------------
* **Dependancy checking** - Done automaticly, no 'setup' needed, just works.
* **Resource file system** - Add files into the final binary as compressed data that can be accessed via a suplied API which includes reading from std::istream.
* **No tempory files created** - Keeps your repository clean from the clutter many tools can add.
* **Project references** - Set dependacies between projects, will build if needed a dependant project.
* **Project date and built time defines** - Creates some defines that can be used in the project to show build time and application version number.
* **Multiple targets such as release / debug / production** - Any dependancies can have their target selected when part of a configuration.
* **Json file format** - Using a Json file format allows other applications such as IDE's to add their own tags without effecting the function of the appbuild tool.

**Motivation**
--------------

I have always found it frustrating using makefiles. I find the syntax hard to use and remeber.
The syntax means that when you go back to a makefile after many months or even years you have to learn it all over again as well as try to workout how your make file works.
For me this makes maintenance hard. Another issue I have had with Makefiles is that I have never got the auto dependency checking to work.
I also want to add features that are only applicable when creating applications such as compiling resource files, automatic date stamping and version marking.

I have tried other tools such as CMake but I did not like the tempory files that these create which can clutter up a repository which in turn can cause issues when working as part of a team.

