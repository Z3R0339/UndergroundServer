workspace "UndergroundServer"
    configurations { "Release" }

project "UndergroundClient"
    kind "SharedLib"
    language "C++"
    cppdialect "C++23"
    location "Client"
    targetdir "bin"
    objdir "obj/client"
    files {
        "Client/*.cpp",

        "Shared/SDK/Basic.cpp",
        "Shared/SDK/CoreUObject_functions.cpp",
        "Shared/SDK/Engine_functions.cpp",
        "Shared/SDK/FortniteGame_functions.cpp",
        "Shared/SDK/GameplayAbilities_functions.cpp",

        "minhook/src/**.c",
    }
    includedirs {
        "Shared",
        "minhook/include"
    }
    buildoptions { "/wd4369", "/wd4309" }
    architecture "x64"

project "UndergroundServer"
    kind "SharedLib"
    language "C++"
    cppdialect "C++23"
    location "Server"
    targetdir "bin"
    objdir "obj/server"
    files {
        "Server/*.cpp",

        "Shared/SDK/Basic.cpp",
        "Shared/SDK/CoreUObject_functions.cpp",
        "Shared/SDK/Engine_functions.cpp",
        "Shared/SDK/FortniteGame_functions.cpp",
        "Shared/SDK/GameplayAbilities_functions.cpp",
        "Shared/SDK/Event_CentralPicnic_Thumper_functions.cpp",
        "Shared/SDK/CentralPicnic_MasterEventController_functions.cpp",

        "minhook/src/**.c",
    }
    includedirs {
        "Shared",
        "minhook/include"
    }
    buildoptions { "/wd4369", "/wd4309" }
    architecture "x64"

