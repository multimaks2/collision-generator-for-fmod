-- Основной workspace для FMOD_GEOMETRY
workspace "FMOD_GEOMETRY"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    preferredtoolarchitecture "x86_64"
    staticruntime "on"
    cppdialect "C++23"
    characterset "Unicode"
    flags "MultiProcessorCompile"
    warnings "Off"
    location "Build"
    targetdir "Bin/%{cfg.architecture}/%{cfg.buildcfg}"
    objdir "Build/obj"

    filter "configurations:Debug"
        defines { "DEBUG", "_DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "NDEBUG" }
        symbols "Off"
        optimize "On"

    filter "system:windows"
        defines { "WIN32", "_WIN32", "_WIN32_WINNT=0x601" }
        buildoptions { "/Zc:__cplusplus", "/permissive-" }
        systemversion "latest"

    filter "toolset:msc"
        buildoptions { "/std:c++latest" }
    
    -- Настройка набора инструментов платформы для Visual Studio
    filter "action:vs2022"
        toolset "v143"
    
    filter "action:vs2019"
        toolset "v142"
    
    filter "action:vs2017"
        toolset "v141"

-- Подключаем модули
include "CORE/premake5.lua"

-- Создаем группу Vendor для отображения в VS (как в MTA)
group "Vendor"
    include "vendor/glfw-master/premake5.lua"
    include "vendor/imgui-master/premake5.lua"
