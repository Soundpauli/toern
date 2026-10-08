cmake_minimum_required(VERSION 3.5)

project(teensypolyphony C CXX)

set(SOURCE_FILES
		sampler.cpp
		stringsampler.cpp
		polyphonicsampler.cpp
		playcontroller/sampleplaymidicontroller.cpp
		loopsampler.cpp)

set(HEADER_FILES
		sampler.h
		stringsampler.h
		polyphonicsampler.h
		TeensyPolyphony.h
		playcontroller/sampleplaymidicontroller.h
		playcontroller/sampleplaymidicontrollerenums.h
		playcontroller/abstractdisplay.h
		playcontroller/serialdisplay.h
		playcontroller/st7735display.h
		playcontroller/extracolors.h
		loopsampler.h
		loopsamplerenums.h)

if (NOT DEFINED TEENSY_VERSION)
	include(CMakeLists.linux.cmake)
else()
	include(CMakeLists.teensy.cmake)
endif()