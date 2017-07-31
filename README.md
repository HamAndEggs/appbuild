# About
This project is an alternative (not a replacement) to makefiles for when compling c/c++ applications. 

So I have taken a leaf out of the Microsoft Visual Studio book and create a stand alone app that uses a single file to represent project consisting of one or more build targets.

The aim is to have a simple file to represent a c/c++ project that is easy to maintain and has all the features that I have wanted over the years. 

## Current features
Targets applications and static libraries.
Full dependency checking.
Project references, if an app refers to a library and the source of that library has changed it will build that library before continuing with building the application, just as you would expect. 
Json file format allowing intergration with external editors where the editor can add it’s own values to the file and the tool will ignore them.

## Motivation
I have always found it frustrating to have to create makefiles. Mostly because the syntax means that when you go back to a makefile after many months or even years you have to learn the syntax all over again. I find their maintability hard. Also I have never got the auto dependency checking to work.

I also want to add features that are only applicable when creating applications such as compiling resource files, automatic date stamping and version marking.

# Installation
Just run the build_and_install.sh and it will compile and copy it to /usr/share/bin and you’re ready to rock and roll.

## Usage
There are some example projects included so you can see how it can be used as well as a way to validate any new build as I work on the project. The project file can be very minimal listing just the files to complile. The other values required will have defaults that will be used. The application has an option, decribed below, that can be used to populate the missing project file options with their defaults.

## Work left to do
* Implement a resource file system.
* Add missing compiler options as and when I discover them or I am informed of them.

## Contributors
Richard e Collins

## License
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
along with this program.  If not, see <http://www.gnu.org/licenses/>.
