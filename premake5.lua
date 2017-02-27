workspace "MulticastGroomer"
  configurations { "Debug", "Release" }
  platforms { "Linux", "Win64" }

project "Groomer"
  kind "ConsoleApp"
  language "C"
  targetdir "bin/%{cfg.buildcfg}"
  architecture "x64"

  files { "**.h", "**.c" } 

  includedirs { "include" }

  links { "event",
          "yaml" }

  filter "platforms:Linux"
    system "linux"

  filter "platforms:Win64"
    system "windows"
    defines { "WIN32" }
    links { "ws2_32" }

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"
