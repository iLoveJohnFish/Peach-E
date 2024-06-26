include "dependencies/conandeps.premake5.lua"

-- premake5.lua
workspace "Peach-E"
    configurations { "Debug", "Release" }
    architecture "x64"

project "Peach Engine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir   "build/%{cfg.buildcfg}/bin"
    objdir        "build/%{cfg.buildcfg}/obj"

    location "./src"
    files { "%{prj.location}/**.h", "%{prj.location}**.cpp" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
    filter{ }

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"  
    filter{ }

    conan_setup()