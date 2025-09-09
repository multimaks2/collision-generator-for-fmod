-- GLFW модуль - внешние зависимости (по образцу MTA)
project "glfw-api"
    language "C"
    kind "StaticLib"
    targetname "glfw-api"
    
    vpaths {
        ["Headers/*"] = {"src/*.h", "include/*.h"},
        ["Sources/*"] = "src/*.c",
        ["*"] = "premake5.lua"
    }
    
    files {
        "src/*.c",
        "src/*.h"
    }
    
    includedirs {
        "include",
        "src"
    }
    
    -- GLFW специфичные настройки
    defines {
        "_GLFW_WIN32",
        "UNICODE",
        "_UNICODE"
    }
    
    -- Исключаем ненужные файлы для Windows
    removefiles {
        "src/glfw_*.c",
        "src/cocoa_*.c",
        "src/linux_*.c",
        "src/posix_*.c",
        "src/x11_*.c"
    }
    
    -- Настройки компилятора
    filter "toolset:msc"
        buildoptions { "/wd4996", "/wd4244" }
    
    filter "toolset:gcc"
        buildoptions { "-Wno-unused-parameter" }
    
    filter "toolset:clang"
        buildoptions { "-Wno-unused-parameter" }
