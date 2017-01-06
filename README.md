## PLEASE NOTE, THIS APP AN EARLY VERSION AND NOT YET FINISHED
## The project file format will be changing. It maybe best to wait for me to finish it. :)

## App Build
This project is an alternative (not a replacement) to makefiles when compling c/c++ applications. 

So I have taken a leaf out of the Microsoft Visual Studio book and create a stand alone app that uses a single file to represent project consisting of one or more build targets.

The aim is to have a simple file to represent a c/c++ project that is easy to maintain and has all the features that I have wanted over the years. 

## Current features
Targest applications.
Full dependency checking.
Json file format allowing intergration with external editors where the editor can add it’s own values to the file and the tool will ignore them.

## Motivation
I have always found it frustrating to have to create makefiles. Mostly because the syntax means that when you go back to a makefile after many months or even years you have to learn the syntax all over again. I find their maintability hard. Also I have never got the auto dependency checking to work.

I also want to add features that are only applicable when creating applications such as compiling resource files, automatic date stamping and version marking.

## Installation
Just run the build_and_install.sh and it will compile and copy it to /usr/share/bin and you’re ready to rock and roll.

# Usage
In time I will create proper documentation. But for now here is an example project file from a project that I am working on for my Raspberry pi. I do plan to add automatic project file creation to get you started. For now just modify this one. The source file entries go in the “groups” object.

{
	"settings":
	{
		"include":
		[
			"/opt/vc/include",
			"/opt/vc/include/interface/vcos/pthreads",
			"../i2c/source",
			"../spi/source",
			"../gpio/source"
		],
		"libpaths":
		[
			"/usr/lib/arm-linux-gnueabihf/",
			"/usr/lib/x86_64-linux-gnu/",
			"/usr/include"
		],
		"libs":
		[
			"stdc++"
		],
		"target":"executable",
		"output_name":"weather",
		"optimisation":0,
		"standard":"c++11"
	},
	"groups":
	{
		"gpio":
		[
			"../gpio/source/gpiomem.cpp"
		],
		"spi":
		[
			"../spi/source/spi_device.cpp",
			"../spi/source/mcp3008.cpp"
		],
		"i2c":
		[
			"../i2c/source/ads1015.cpp",
			"../i2c/source/i2c_device.cpp",
			"../i2c/source/tmp102.cpp",
			"../i2c/source/bmp280.cpp",
			"../i2c/source/si1145.cpp",
			"../i2c/source/tsl2561.cpp"
		],
		"Source":
		[
			"./source/weather.cpp"
		]
	}
}

## Contributors
Richard e Collin

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
