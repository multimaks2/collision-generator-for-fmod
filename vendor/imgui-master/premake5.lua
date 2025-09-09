-- ImGui модуль - header-only UI библиотека
project "imgui"
    language "C++"
    kind "StaticLib"
    targetname "imgui"
    
    vpaths {
        ["Headers/*"] = {"*.h", "*.hpp"},
        ["Sources/*"] = "*.cpp",
        ["*"] = "premake5.lua"
    }
    
    files {
        "imgui.cpp",
        "imgui.h",
        "imgui_demo.cpp",
        "imgui_draw.cpp",
        "imgui_tables.cpp",
        "imgui_widgets.cpp",
        "backends/imgui_impl_glfw.cpp",
        "backends/imgui_impl_glfw.h",
        "backends/imgui_impl_opengl3.cpp",
        "backends/imgui_impl_opengl3.h"
    }
    
    includedirs {
        ".",
        "backends",
        "../glfw-master/include"  -- Путь к GLFW заголовкам
    }
    
    -- ImGui специфичные настройки
    defines {
        "IMGUI_DISABLE_OBSOLETE_FUNCTIONS"
        -- Убираем IMGUI_DISABLE_DEMO_WINDOWS для демо
    }
    
    -- Настройки компилятора
    filter "toolset:msc"
        buildoptions { "/wd4996", "/wd4244" }
    
    filter "toolset:gcc"
        buildoptions { "-Wno-unused-parameter" }
    
    filter "toolset:clang"
        buildoptions { "-Wno-unused-parameter" }
