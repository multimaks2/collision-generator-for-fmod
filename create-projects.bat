@echo off
echo Creating projects with premake5...

REM Проверяем наличие premake5.exe
if not exist "utils\premake5.exe" (
    echo Error: premake5.exe not found in utils folder!
    echo Please make sure premake5.exe is available in the utils directory.
    pause
    exit /b 1
)

REM Создаем папки для выходных файлов если их нет
if not exist "Bin" mkdir Bin
if not exist "Bin\x86_64" mkdir Bin\x86_64
if not exist "Bin\x86_64\Debug" mkdir Bin\x86_64\Debug
if not exist "Bin\x86_64\Release" mkdir Bin\x86_64\Release

if not exist "Build" mkdir Build
if not exist "Build\obj" mkdir Build\obj

REM Генерируем проект для Visual Studio 2022
echo Generating Visual Studio 2022 solution...
utils\premake5.exe vs2022



REM Генерируем проект для CodeBlocks
echo Generating CodeBlocks project...
utils\premake5.exe gmake

REM Копируем .sln файл в корень проекта и исправляем пути
echo.
echo Copying and fixing solution file...
if exist "Build\FMOD_GEOMETRY.sln" (
    copy "Build\FMOD_GEOMETRY.sln" "FMOD_GEOMETRY.sln"
    
    REM Исправляем пути ко всем .vcxproj файлам
    powershell -Command "(Get-Content 'FMOD_GEOMETRY.sln') -replace 'Core\.vcxproj', 'Build\\Core.vcxproj' | Set-Content 'FMOD_GEOMETRY.sln'"
    powershell -Command "(Get-Content 'FMOD_GEOMETRY.sln') -replace 'glfw-api\.vcxproj', 'Build\\glfw-api.vcxproj' | Set-Content 'FMOD_GEOMETRY.sln'"
    powershell -Command "(Get-Content 'FMOD_GEOMETRY.sln') -replace 'imgui\.vcxproj', 'Build\\imgui.vcxproj' | Set-Content 'FMOD_GEOMETRY.sln'"
    
    echo Solution file copied and fixed
) else (
    echo Warning: Solution file not found in Build folder
)

echo.
echo Projects generated successfully!
echo.
echo Available solutions:
echo - FMOD_GEOMETRY.sln (Visual Studio - in project root)
echo - FMOD_GEOMETRY.workspace (CodeBlocks)
echo.
echo Output directories:
echo - Bin\x86_64\Debug\   (Debug executable files)
echo - Bin\x86_64\Release\ (Release executable files)
echo - Build\              (Project files: .vcxproj, .make, .obj)
echo - Root\                (Solution file: .sln - fixed paths)
echo.
echo You can now open the solution file in Visual Studio or CodeBlocks.
echo.
pause
