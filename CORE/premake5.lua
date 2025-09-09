-- CORE модуль - основной исполняемый файл
project "Core"
    kind "WindowedApp"
    language "C++"
    warnings "Extra"
    
    vpaths {
        ["Headers/*"] = {"**.h", "**.hpp"},
        ["Sources/*"] = "**.cpp",
        ["*"] = "premake5.lua"
    }
    
    files {
        "**.cpp",
        "**.h",
        "**.hpp"
    }
    
                                                     includedirs {
                ".",
                "RW",
                "../vendor/glfw-master/include",
                "../vendor/glm-master",
                "../vendor/imgui-master",
                "../vendor/glew-2.2.0/include"
            }
            
            -- Зависимости от vendor модулей
            links { "glfw-api", "imgui" }
            
            -- GLEW библиотека (статическая линковка)
            libdirs { "../vendor/glew-2.2.0/lib/Release/x64" }
            links { "glew32s" }
            
            -- Определения для статической линковки GLEW
            defines { "GLEW_STATIC" }
    
    -- OpenGL библиотеки для Windows
    filter "system:windows"
        links { "opengl32" }
    
         -- Определения для модуля
     defines {
         "FMOD_GEOMETRY_EXPORTS",
         "GLM_FORCE_RADIANS",
         "GLM_FORCE_DEPTH_ZERO_TO_ONE",
         "GLEW_STATIC"
     }
    
    -- Настройки компилятора
    filter "toolset:msc"
        buildoptions { "/W4", "/wd4251", "/wd4275" }
    
    filter "toolset:gcc"
        buildoptions { "-Wall", "-Wextra", "-Wpedantic" }
    
    filter "toolset:clang"
        buildoptions { "-Wall", "-Wextra", "-Wpedantic" }
