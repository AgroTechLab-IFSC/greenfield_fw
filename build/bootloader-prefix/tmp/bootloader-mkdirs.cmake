# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/espressif/v5.2/esp-idf/components/bootloader/subproject"
  "C:/AgroTechLab/AgTech4All/ats_04_fw/build/bootloader"
  "C:/AgroTechLab/AgTech4All/ats_04_fw/build/bootloader-prefix"
  "C:/AgroTechLab/AgTech4All/ats_04_fw/build/bootloader-prefix/tmp"
  "C:/AgroTechLab/AgTech4All/ats_04_fw/build/bootloader-prefix/src/bootloader-stamp"
  "C:/AgroTechLab/AgTech4All/ats_04_fw/build/bootloader-prefix/src"
  "C:/AgroTechLab/AgTech4All/ats_04_fw/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/AgroTechLab/AgTech4All/ats_04_fw/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/AgroTechLab/AgTech4All/ats_04_fw/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
