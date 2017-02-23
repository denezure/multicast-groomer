workspace "MulticastGroomer"
  configurations { "Debug", "Release" }

project "GroomerTest"
  kind "ConsoleApp"
  language "C"
  targetdir "bin/%{cfg.buildcfg}"

  files { "**.h", "**.c" } 

  includedirs { "include" }

  defines { "TESTBUILD" }

  -- Criterion unit test library
  links { "criterion",
          "event",
          "yaml" }

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"
