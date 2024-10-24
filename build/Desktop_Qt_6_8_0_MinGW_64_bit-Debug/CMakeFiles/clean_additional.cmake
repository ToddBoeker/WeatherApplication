# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\WeatherApplication_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\WeatherApplication_autogen.dir\\ParseCache.txt"
  "WeatherApplication_autogen"
  )
endif()
