#!lua
local output = "./build/" .. _ACTION

solution "smsplus-gx_solution"
   configurations { "Debug", "Release" }

project "core"
   location (output)
   kind "StaticLib"
   language "C"
   includedirs { "core/.", "core/cpu", "core/sound", "core/unzip" }
   files { "core/**.h", "core/**.c" }
   buildoptions { "-Wall" }
   defines { "LSB_FIRST=1" }

   configuration "Debug"
      flags { "Symbols" }
      defines { "DEBUG" }

   configuration "Release"
      flags { "Optimize" }
      defines { "NDEBUG" }

project "smsplus-gx"
   location (output)
   kind "ConsoleApp"
   language "C"
   includedirs { "./core", "core/cpu", "core/sound", "core/unzip", "/usr/include/libdrm" }
   files { "./*.h", "./*.c"}
   buildoptions { "-Wall" }
   linkoptions { "-lpthread -lm -lrga -lgo2" }
   links {"core"}
   defines { "LSB_FIRST=1" }
   
   configuration "Debug"
      flags { "Symbols" }
      defines { "DEBUG" }

   configuration "Release"
      flags { "Optimize" }
      defines { "NDEBUG" }
