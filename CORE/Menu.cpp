#include "Menu.h"
#include "Renderer.h"
#include "Logger.h"
#include <windows.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <fstream>

Menu::Menu() : m_window(nullptr), m_renderer(nullptr), m_initialized(false), m_fontLoaded(false), m_smallFont(nullptr),
    m_cameraX(0.0f), m_cameraY(0.0f), m_cameraZ(0.0f),
    m_cameraRotY(0.0f), m_fov(90.0f),
    m_cameraSpeed(50.0f),
    m_cameraSpeedChanged(false), m_newCameraSpeed(50.0f),
    m_renderRadiusChanged(false), m_newRenderRadius(1500.0f),
    m_imguiWidthPercent(0.3f), m_imguiHeightPercent(0.4f),
    m_showMainOverlay(true), m_showStatsBar(true), m_showCameraInfo(true),
    m_autoScroll(true), m_maxLogEntries(1000),
    m_lastFrameTime(0.0), m_frameTime(0.0), m_avgFrameTime(0.0), m_frameCount(0),
    m_fullscreen(false), m_windowedWidth(1280), m_windowedHeight(720), m_windowedX(100), m_windowedY(100),
    m_useQuaternions(true), m_rotationModeChanged(false) {
    
}

Menu::~Menu() {
    Shutdown();
}

void Menu::Initialize(GLFWwindow* window) {
    if (m_initialized) return;
    
    m_window = window;
    
    // Получаем размеры окна
    glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    
    // Устанавливаем callback для изменения размера окна и синхронизации полноэкранного режима
    glfwSetWindowSizeCallback(m_window, WindowSizeCallback);
    
    // Устанавливаем user pointer для доступа к Menu в callback
    glfwSetWindowUserPointer(m_window, this);
    
    // Настраиваем ImGui
    SetupImGui();
    
    // Устанавливаем экземпляр Menu в глобальной системе логирования
    SetMenuInstance(static_cast<void*>(this), &Menu::StaticAddLog);
     
    // Добавляем тестовые логи
    AddLog("Система меню успешно инициализирована", "System", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    AddLog("ImGui стили настроены", "System", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    AddLog("Консоль логов готова к работе", "Debug", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    
    // Тестируем глобальные функции логирования
    //LogSystem("Глобальная система логирования инициализирована");
    //LogRender("Тест логирования рендеринга");
    //LogImg("Тест логирования IMG");
    //LogCamera("Тест логирования камеры");
    //LogModels("Тест логирования моделей");
    //LogDebug("Тест отладочного логирования");
    
    m_initialized = true;
    LogSystem("Система меню успешно инициализирована");
}

void Menu::Shutdown() {
    if (!m_initialized) return;
    
    if (m_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }
    
    LogSystem("Система меню завершена");
}

void Menu::BeginFrame() {
    if (!m_initialized || !m_window) return;
    
    // Обновляем FPS
    UpdateFPS();
    
    // Обработка клавиши F11 для переключения полноэкранного режима
    static bool f11Pressed = false;
    
    if (glfwGetKey(m_window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!f11Pressed) {
            printf("[Debug] F11 нажата, вызываем ToggleFullscreen\n");
            ToggleFullscreen();
            f11Pressed = true;
        }
    } else {
        f11Pressed = false;
    }
    
    // Обработка клавиши R для переключения режима поворота
    static bool rPressed = false;
    
    if (glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!rPressed) {
            printf("[Debug] R нажата, вызываем ToggleRotationMode\n");
            ToggleRotationMode();
            rPressed = true;
        }
    } else {
        rPressed = false;
    }
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Menu::Render() {
    if (!m_initialized || !m_window) return;
    
    // Рендерим Performance Monitor
    RenderPerformanceMonitor();
    

}

void Menu::EndFrame() {
    if (!m_initialized || !m_window) return;
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Menu::SetRenderer(Renderer* renderer) {
    m_renderer = renderer;
}

void Menu::SetCameraInfo(float x, float y, float z, float rotX, float rotY, float fov) {
    m_cameraX = x;
    m_cameraY = y;
    m_cameraZ = z;
    m_cameraRotX = rotX;
    m_cameraRotY = rotY;
    m_fov = fov;
    
    // Обновляем позицию камеры (логирование убрано)
    static float lastX = x, lastY = y, lastZ = z;
    if (abs(x - lastX) > 1.0f || abs(y - lastY) > 1.0f || abs(z - lastZ) > 1.0f) {
        lastX = x; lastY = y; lastZ = z;
    }
}

void Menu::SetCameraSpeed(float baseSpeed) {
    m_cameraSpeed = baseSpeed;
}



void Menu::ResetRequestFlags() {
    // Сбрасываем флаги настроек
    m_cameraSpeedChanged = false;
    m_renderRadiusChanged = false;
    m_rotationModeChanged = false;
}

void Menu::OnWindowResize(int width, int height) {
    // Обновляем размеры окна
    m_windowWidth = width;
    m_windowHeight = height;
    
    // Логируем изменение размера окна
    //AddLog("Размер окна изменен на " + std::to_string(width) + "x" + std::to_string(height), "System", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
    
    // При изменении размера окна все ImGui окна автоматически пересчитают свои размеры
    // в следующем кадре в методе RenderPerformanceMonitor()
}

void Menu::OnCameraSpeedChanged(float newSpeed) {
    m_newCameraSpeed = newSpeed;
    m_cameraSpeedChanged = true;
    
    // Логируем изменение скорости камеры
    //AddLog("Скорость камеры изменена на " + std::to_string((int)newSpeed), "System", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
}

void Menu::OnRenderRadiusChanged(float newRadius) {
    m_newRenderRadius = newRadius;
    m_renderRadiusChanged = true;
    
    // Логируем изменение радиуса рендеринга
    //AddLog("Радиус рендеринга изменен на " + std::to_string((int)newRadius), "Render", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
}

// ====================================================================================================
// ПРИВАТНЫЕ МЕТОДЫ
// ====================================================================================================

void Menu::SetupImGui() {
    // Базовая инициализация ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    // Настраиваем ImGui для поддержки кириллицы
    ImGuiIO& io = ImGui::GetIO();
    
    // Добавляем диапазон символов для кириллицы
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x04FF, // Cyrillic
        0x0500, 0x052F, // Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    
    // Загружаем системный шрифт Arial с поддержкой кириллицы
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 16.0f, nullptr, ranges);
    
    // Устанавливаем загруженный шрифт как основной
    if (font) {
        io.FontDefault = font;
        m_fontLoaded = true;
        LogSystem("Шрифт Arial с поддержкой кириллицы успешно загружен");
    } else {
        m_fontLoaded = false;
        LogError("Не удалось загрузить шрифт Arial");
    }
    
    // Загружаем маленький шрифт для консоли логов (50% от основного)
    m_smallFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 10.0f, nullptr, ranges);
    if (m_smallFont) {
        LogSystem("Маленький шрифт для консоли логов успешно загружен");
    } else {
        LogError("Не удалось загрузить маленький шрифт для консоли");
        m_smallFont = font; // Используем основной шрифт как fallback
    }
    
    // Уменьшаем шрифт для компактности
    io.FontGlobalScale = 0.8f;
    
    // Настраиваем стиль
    SetupStyle();
    
    // Инициализация бэкендов
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true)) {
        m_initialized = false;
        return;
    }
    
    if (!ImGui_ImplOpenGL3_Init("#version 120")) {
        m_initialized = false;
        return;
    }
    
    m_initialized = true;
}

void Menu::SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Основные размеры и настройки
    style.WindowMinSize        = ImVec2(160, 20);
    style.FramePadding         = ImVec2(4, 2);
    style.ItemSpacing          = ImVec2(6, 2);
    style.ItemInnerSpacing     = ImVec2(6, 4);
    style.Alpha                = 0.95f;
    style.WindowRounding       = 4.0f;
    style.FrameRounding        = 2.0f;
    style.IndentSpacing        = 6.0f;
    style.ColumnsMinSpacing    = 50.0f;
    style.GrabMinSize          = 14.0f;
    style.GrabRounding         = 16.0f;
    style.ScrollbarSize        = 12.0f;
    style.ScrollbarRounding    = 16.0f;
    
    // Дополнительные настройки для совместимости
    style.ChildRounding        = 4.0f;
    style.TabRounding          = 4.0f;
    style.WindowPadding        = ImVec2(6, 6);
    style.DisabledAlpha        = 0.6f;
    
    // Цветовая схема - темная тема
    ImVec4* colors = style.Colors;
    
    // Основные цвета
    colors[ImGuiCol_Text]                  = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
    //colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.22f, 0.27f, 0.73f);
    
    // Дополнительные цвета для совместимости
    colors[ImGuiCol_Tab]                   = ImVec4(0.20f, 0.22f, 0.27f, 0.86f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.92f, 0.18f, 0.29f, 0.80f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.92f, 0.18f, 0.29f, 0.84f);
}

void Menu::RenderPerformanceMonitor() {
    // Получаем текущие размеры окна
    int currentWidth, currentHeight;
    glfwGetWindowSize(m_window, &currentWidth, &currentHeight);
    
    // Обновляем размеры окна
    m_windowWidth = currentWidth;
    m_windowHeight = currentHeight;
    

    
    // 2. ПОЛОСА СТАТИСТИКИ ВНИЗУ (по всей ширине экрана)
    if (m_showStatsBar) {
        float statsBarHeight = 85.0f;
        ImGui::SetNextWindowPos(ImVec2(0, currentHeight - statsBarHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(currentWidth, statsBarHeight), ImGuiCond_Always);
        
        ImGuiWindowFlags statsBarFlags = ImGuiWindowFlags_NoTitleBar | 
                                       ImGuiWindowFlags_NoResize | 
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        // Устанавливаем прозрачность через цвет фона
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        
        ImGui::Begin("StatsBar", nullptr, statsBarFlags);
        
        // Разделяем полосу на секции
        float sectionWidth = currentWidth / 5.0f;
        
        // Секция 1: Статистика DFF рендеринга (без кубмапа)
        ImGui::BeginChild("DffRenderStats", ImVec2(sectionWidth - 10, statsBarHeight - 15), true);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "DFF РЕНДЕРИНГ");
        ImGui::Text("Полигоны: %d", m_renderer->GetDffPolygons());
        ImGui::Text("Вершины: %d", m_renderer->GetDffVertices());
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Секция 2: Статистика кубмапа (skybox)
        ImGui::BeginChild("SkyboxStats", ImVec2(sectionWidth - 10, statsBarHeight - 15), true);
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f), "КУБМАП");
        ImGui::Text("Полигоны: %d", m_renderer->GetSkyboxPolygons());
        ImGui::Text("Вершины: %d", m_renderer->GetSkyboxVertices());
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Секция 3: Радиус рендеринга
        ImGui::BeginChild("RenderRadius", ImVec2(sectionWidth - 10, statsBarHeight - 15), true);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "РАДИУС РЕНДЕРИНГА");
    float renderRadius = m_renderer->GetRenderRadius();
        if (ImGui::SliderFloat("##Radius", &renderRadius, 100.0f, 3000.0f, "%.0f")) {
        m_renderer->SetRenderRadius(renderRadius);
        OnRenderRadiusChanged(renderRadius);
    }
        ImGui::EndChild();
    
        ImGui::SameLine();
    
        // Секция 4: Скорость камеры
        ImGui::BeginChild("CameraSpeed", ImVec2(sectionWidth - 10, statsBarHeight - 15), true);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "СКОРОСТЬ КАМЕРЫ");
    float speed = m_cameraSpeed;
    if (ImGui::SliderFloat("##Speed", &speed, 100.0f, 3000.0f, "%.0f")) {
        m_cameraSpeed = speed;
        OnCameraSpeedChanged(speed);
    }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Секция 5: Информация о моделях
        ImGui::BeginChild("ModelInfo", ImVec2(sectionWidth - 10, statsBarHeight - 15), true);
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "МОДЕЛИ");
        ImGui::Text("DFF моделей: %d", m_renderer->GetDffModelCount());
        ImGui::Text("GTA объектов: %d", m_renderer->GetGtaObjectCount());
        ImGui::EndChild();
        
        ImGui::End();
        
        // Восстанавливаем цвет фона
        ImGui::PopStyleColor();
    }
    
    // 3. ОКНО КООРДИНАТ КАМЕРЫ (правый верхний угол)
    if (m_showCameraInfo) {
        float cameraWindowWidth = 120.0f;
        float cameraWindowHeight = 137.5f;
        // Позиционируем окно камеры в правом верхнем углу с отступом 10px от правого края
        ImGui::SetNextWindowPos(ImVec2( (currentWidth- cameraWindowWidth) - 50 , 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(cameraWindowWidth, cameraWindowHeight), ImGuiCond_Always);
        
        ImGuiWindowFlags cameraWindowFlags = ImGuiWindowFlags_NoTitleBar | 
                                           ImGuiWindowFlags_NoResize | 
                                           ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        // Устанавливаем прозрачность через цвет фона
        //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.07f, 0.9f));
        //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.9f));
        
        ImGui::Begin("CameraInfo", nullptr, cameraWindowFlags);
        
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "КАМЕРА");
        ImGui::Text("X: %.1f", m_cameraX);
        ImGui::Text("Y: %.1f", m_cameraY);
        ImGui::Text("Z: %.1f", m_cameraZ);
        ImGui::Text("Поворот X: %.1f°", m_cameraRotX);
        ImGui::Text("Поворот Y: %.1f°", m_cameraRotY);
        ImGui::Text("FOV: %.1f°", m_fov);
        ImGui::End();
        
        // Восстанавливаем цвет фона
        //ImGui::PopStyleColor();
        
        // ОКНО FPS (под окном камеры)
        float fpsWindowWidth = 120.0f;
        float fpsWindowHeight = 85.0f;
        // Позиционируем окно FPS под окном камеры с теми же координатами X
        ImGui::SetNextWindowPos(ImVec2((currentWidth - cameraWindowWidth) - 50, 10 + cameraWindowHeight + 5), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(fpsWindowWidth, fpsWindowHeight), ImGuiCond_Always);
        
        ImGuiWindowFlags fpsWindowFlags = ImGuiWindowFlags_NoTitleBar | 
                                        ImGuiWindowFlags_NoResize | 
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        // Устанавливаем прозрачность через цвет фона
        //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.07f, 0.9f));
        
        ImGui::Begin("FPSInfo", nullptr, fpsWindowFlags);
        
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS");
        
        // Вычисляем FPS
        int currentFPS = (m_frameTime > 0.0) ? (int)(1.0 / m_frameTime) : 0;
        int avgFPS = (m_avgFrameTime > 0.0) ? (int)(1.0 / m_avgFrameTime) : 0;
        
        ImGui::Text("FPS: %d", currentFPS);
        ImGui::Text("[%.1fms]", m_frameTime * 1000.0);
        ImGui::Text("AVG: %d", avgFPS);
        
        ImGui::End();
        
        // Восстанавливаем цвет фона
        //ImGui::PopStyleColor();
    }
    
    // 4. ПАНЕЛЬ УПРАВЛЕНИЯ HUD (левый верхний угол)
    float hudControlWidth = 300.0f;
    float hudControlHeight = 120.0f; // Увеличиваем высоту для новой кнопки, информации и горячих клавиш
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hudControlWidth, hudControlHeight), ImGuiCond_Always);
    
    ImGuiWindowFlags hudControlFlags = ImGuiWindowFlags_NoTitleBar | 
                                     ImGuiWindowFlags_NoResize | 
                                     ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    // Устанавливаем прозрачность через цвет фона
    //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.07f, 0.8f));
    
    ImGui::Begin("HUDControl", nullptr, hudControlFlags);
    
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "ИНТЕРФЕЙС");

    if (ImGui::Button(m_useQuaternions ? "КВАТЕРНИОНЫ [R]" : "УГЛЫ ЭЙЛЕРА [R]", ImVec2(150, 25))) {
        ToggleRotationMode();
    }
    ImGui::SameLine();
    if (ImGui::Button("DUMP GEOMETRY", ImVec2(150, 25))) {
        DumpGeometry();
    }
    ImGui::Text("Текущий: %s", m_useQuaternions ? "Кватернионы (rx,ry,rz,rw)" : "Углы Эйлера (rx,ry,rz)");
    ImGui::Checkbox("Полоса статистики", &m_showStatsBar);
    ImGui::Checkbox("Координаты камеры и FPS", &m_showCameraInfo);

    ImGui::End();
    
    // Восстанавливаем цвет фона
    //ImGui::PopStyleColor();
    
    // 5. ОКНО КОНСОЛИ (под панелью интерфейса)
    float consoleWidth = 400.0f;
    float consoleHeight = 200.0f;
    ImGui::SetNextWindowPos(ImVec2(10, 10 + 120 + 10), ImGuiCond_Always); // Обновляем позицию для новой высоты HUD
    ImGui::SetNextWindowSize(ImVec2(consoleWidth, consoleHeight), ImGuiCond_Always);
    
    ImGuiWindowFlags consoleFlags = ImGuiWindowFlags_NoTitleBar | 
                                  ImGuiWindowFlags_NoResize | 
                                  ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    // Устанавливаем прозрачность через цвет фона
    //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.07f, 0.9f));
    
    ImGui::Begin("Console", nullptr, consoleFlags);
    
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "КОНСОЛЬ ЛОГОВ");
    ImGui::Separator();
    
    // Вкладки для различных типов логов
    static int selectedTab = 0;
    const char* tabs[] = { "Render", "System", "IMG", "IPL", "COL", "Models", "Debug" };
    
    if (ImGui::BeginTabBar("LogTabs")) {
        for (int i = 0; i < IM_ARRAYSIZE(tabs); i++) {
            if (ImGui::BeginTabItem(tabs[i])) {
                selectedTab = i;
                
                // Область для отображения логов
                ImGui::BeginChild("LogContent", ImVec2(0, consoleHeight - 80), true);
                
                // Устанавливаем маленький шрифт для консоли логов
                if (m_smallFont) {
                    ImGui::PushFont(m_smallFont);
                }
                
                // Отображаем логи в зависимости от выбранной вкладки
                std::lock_guard<std::mutex> lock(m_logMutex);
                
                // Фильтруем логи по категории
                std::vector<LogEntry> filteredLogs;
                for (const auto& log : m_logs) {
                    bool showLog = false;
                    switch (selectedTab) {
                        case 0: // Render
                            showLog = (log.category == "Render" || log.category == "General");
                            break;
                        case 1: // System
                            showLog = (log.category == "System" || log.category == "General");
                            break;
                        case 2: // IMG
                            showLog = (log.category == "IMG" || log.category == "General");
                            break;
                        case 3: // IPL
                            showLog = (log.category == "IPL" || log.category == "General");
                            break;
                        case 4: // COL
                            showLog = (log.category == "COL" || log.category == "General");
                            break;
                        case 5: // Models
                            showLog = (log.category == "Models" || log.category == "General");
                            break;
                        case 6: // Debug
                            showLog = (log.category == "Debug" || log.category == "General");
                            break;
                    }
                    
                    if (showLog) {
                        filteredLogs.push_back(log);
                    }
                }
                
                // Отображаем отфильтрованные логи
                for (const auto& log : filteredLogs) {
                    ImGui::TextColored(log.color, "[%s] %s", log.category.c_str(), log.message.c_str());
                }
                
                // Автопрокрутка
                if (m_autoScroll) {
                    ImGui::SetScrollHereY(1.0f);
                }
                
                // Восстанавливаем основной шрифт
                if (m_smallFont) {
                    ImGui::PopFont();
                }
                
                ImGui::EndChild();
                
                // Кнопки управления логами
                ImGui::Separator();
                
                // Устанавливаем маленький шрифт для кнопок управления логами
                if (m_smallFont) {
                    ImGui::PushFont(m_smallFont);
                }
                
                if (ImGui::Button("Очистить логи")) {
                    try {
                        ClearLogs();
                    } catch (...) {
                        printf("[Error] Критическая ошибка при очистке логов\n");
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Экспорт логов")) {
                    try {
                        ExportLogs();
                    } catch (...) {
                        printf("[Error] Критическая ошибка при экспорте логов\n");
                    }
                }
                ImGui::SameLine();
                
                // Определяем категорию для копирования на основе выбранной вкладки
                std::string copyCategory = "";
                if (selectedTab >= 0 && selectedTab < 7) {
                    const char* categories[] = { "Render", "System", "IMG", "IPL", "COL", "Models", "Debug" };
                    copyCategory = categories[selectedTab];
                }
                
                if (ImGui::Button("Копировать в буфер")) {
                    try {
                        CopyLogsToClipboard(copyCategory);
                    } catch (...) {
                        printf("[Error] Критическая ошибка при копировании логов\n");
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Обновить")) {
                    // Принудительное обновление логов
                }
                ImGui::SameLine();
                ImGui::Checkbox("Автопрокрутка", &m_autoScroll);
                
                // Восстанавливаем основной шрифт после кнопок
                if (m_smallFont) {
                    ImGui::PopFont();
                }
                
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    
    ImGui::End();
    
    // Восстанавливаем цвет фона
    //ImGui::PopStyleColor();
}

// Методы логирования
void Menu::AddLog(const std::string& message, const std::string& category, ImVec4 color) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    
    LogEntry entry;
    entry.message = message;
    entry.category = category;
    entry.color = color;
    
    m_logs.push_back(entry);
    
    // Ограничиваем количество логов
    if (m_logs.size() > m_maxLogEntries) {
        m_logs.erase(m_logs.begin());
    }
}

void Menu::ClearLogs() {
    try {
        // Проверяем, что мьютекс доступен
        if (m_logMutex.try_lock()) {
            m_logs.clear();
            m_logMutex.unlock();
            
            // Используем printf для отладки, так как LogSystem может не работать
            printf("[System] Логи успешно очищены\n");
        } else {
            printf("[Error] Не удалось заблокировать мьютекс для очистки логов\n");
        }
    } catch (const std::exception& e) {
        printf("[Error] Ошибка при очистке логов: %s\n", e.what());
    } catch (...) {
        printf("[Error] Неизвестная ошибка при очистке логов\n");
    }
}

void Menu::ExportLogs() {
    try {
        // Проверяем, что мьютекс доступен
        if (m_logMutex.try_lock()) {
            std::ofstream logFile("fmod_geometry_logs.txt");
            if (logFile.is_open()) {
                logFile << "=== FMOD Geometry Viewer Logs ===\n";
                logFile << "================================\n\n";
                
                for (const auto& log : m_logs) {
                    logFile << "[" << log.category << "] " << log.message << "\n";
                }
                
                logFile.close();
                printf("[System] Логи экспортированы в fmod_geometry_logs.txt\n");
            } else {
                printf("[Error] Не удалось создать файл для экспорта логов\n");
            }
            m_logMutex.unlock();
        } else {
            printf("[Error] Не удалось заблокировать мьютекс для экспорта логов\n");
        }
    } catch (const std::exception& e) {
        printf("[Error] Ошибка при экспорте логов: %s\n", e.what());
    } catch (...) {
        printf("[Error] Неизвестная ошибка при экспорте логов\n");
    }
}

void Menu::CopyLogsToClipboard(const std::string& category) {
    try {
        // Проверяем, что мьютекс доступен
        if (m_logMutex.try_lock()) {
            std::string clipboardText;
            
            if (category.empty()) {
                // Копируем все логи
                for (const auto& log : m_logs) {
                    clipboardText += "[" + log.category + "] " + log.message + "\n";
                }
            } else {
                // Копируем логи только указанной категории
                for (const auto& log : m_logs) {
                    if (log.category == category || log.category == "General") {
                        clipboardText += "[" + log.category + "] " + log.message + "\n";
                    }
                }
            }
            
            m_logMutex.unlock();
            
            if (!clipboardText.empty()) {
                // Открываем буфер обмена
                if (OpenClipboard(nullptr)) {
                    // Очищаем буфер
                    EmptyClipboard();
                    
                    // Выделяем память для текста
                    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, clipboardText.size() + 1);
                    if (hGlobal) {
                        // Копируем текст в выделенную память
                        char* pData = static_cast<char*>(GlobalLock(hGlobal));
                        if (pData) {
                            strcpy_s(pData, clipboardText.size() + 1, clipboardText.c_str());
                            GlobalUnlock(hGlobal);
                            
                            // Устанавливаем данные в буфер обмена
                            if (SetClipboardData(CF_TEXT, hGlobal)) {
                                printf("[System] Логи скопированы в буфер обмена\n");
                            } else {
                                printf("[Error] Не удалось установить данные в буфер обмена\n");
                                GlobalFree(hGlobal);
                            }
                        } else {
                            GlobalFree(hGlobal);
                            printf("[Error] Не удалось заблокировать память для буфера обмена\n");
                        }
                    } else {
                        printf("[Error] Не удалось выделить память для буфера обмена\n");
                    }
                    
                    CloseClipboard();
                } else {
                    printf("[Error] Не удалось открыть буфер обмена\n");
                }
            } else {
                printf("[Warning] Нет логов для копирования\n");
            }
        } else {
            printf("[Error] Не удалось заблокировать мьютекс для копирования логов\n");
        }
    } catch (const std::exception& e) {
        printf("[Error] Ошибка при копировании логов: %s\n", e.what());
    } catch (...) {
        printf("[Error] Неизвестная ошибка при копировании логов\n");
    }
}

void Menu::UpdateFPS() {
    double currentTime = glfwGetTime();
    
    // Вычисляем время кадра
    m_frameTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;
    
    // Обновляем среднее время кадра
    m_avgFrameTime = (m_avgFrameTime * m_frameCount + m_frameTime) / (m_frameCount + 1);
    if (m_frameCount < FPS_SAMPLE_COUNT) {
        m_frameCount++;
    }
}

void Menu::ToggleFullscreen() {
    if (!m_window) return;
    
    // Получаем текущее состояние полноэкранного режима от GLFW
    GLFWmonitor* currentMonitor = glfwGetWindowMonitor(m_window);
    bool isCurrentlyFullscreen = (currentMonitor != nullptr);
    
    // Определяем, что нужно сделать: перейти в полноэкранный или выйти
    bool shouldGoFullscreen = !isCurrentlyFullscreen;
    
    printf("[Debug] ToggleFullscreen: текущее состояние GLFW: %s, нужно переключить на: %s\n", 
           isCurrentlyFullscreen ? "полноэкранный" : "оконный",
           shouldGoFullscreen ? "полноэкранный" : "оконный");
    
    if (shouldGoFullscreen) {
        // Переходим в полноэкранный режим
        // Сохраняем текущие параметры окна
        glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
        glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);
        
        printf("[Debug] Сохранены параметры окна: %dx%d в позиции (%d, %d)\n", 
               m_windowedWidth, m_windowedHeight, m_windowedX, m_windowedY);
        
        // Получаем монитор (используем текущий монитор окна, если возможно)
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        
        // Попробуем найти монитор, на котором находится окно
        int windowX, windowY;
        glfwGetWindowPos(m_window, &windowX, &windowY);
        
        // Ищем ближайший монитор к позиции окна
        int monitorCount;
        GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
        GLFWmonitor* bestMonitor = monitor; // По умолчанию используем основной
        
        for (int i = 0; i < monitorCount; i++) {
            int monitorX, monitorY;
            glfwGetMonitorPos(monitors[i], &monitorX, &monitorY);
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
            if (mode) {
                // Проверяем, находится ли окно на этом мониторе
                if (windowX >= monitorX && windowX < monitorX + mode->width &&
                    windowY >= monitorY && windowY < monitorY + mode->height) {
                    bestMonitor = monitors[i];
                    printf("[Debug] Найден монитор для окна: %s\n", glfwGetMonitorName(bestMonitor));
                    break;
                }
            }
        }
        
        if (bestMonitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(bestMonitor);
            if (mode) {
                glfwSetWindowMonitor(m_window, bestMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                m_fullscreen = true;
                printf("[System] Переключен в полноэкранный режим: %dx%d на мониторе %s\n", 
                       mode->width, mode->height, glfwGetMonitorName(bestMonitor));
            }
        }
    } else {
        // Возвращаемся в оконный режим
        printf("[Debug] Возвращаемся в оконный режим: %dx%d в позиции (%d, %d)\n", 
               m_windowedWidth, m_windowedHeight, m_windowedX, m_windowedY);
        
        glfwSetWindowMonitor(m_window, nullptr, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight, 0);
        m_fullscreen = false;
        printf("[System] Возвращен оконный режим: %dx%d в позиции (%d, %d)\n", 
               m_windowedWidth, m_windowedHeight, m_windowedX, m_windowedY);
    }
    
    // Синхронизируем нашу переменную с реальным состоянием GLFW после переключения
    GLFWmonitor* newMonitor = glfwGetWindowMonitor(m_window);
    bool newFullscreenState = (newMonitor != nullptr);
    m_fullscreen = newFullscreenState;
    
    printf("[Debug] После переключения: GLFW состояние: %s, наша переменная: %s\n",
           newFullscreenState ? "полноэкранный" : "оконный",
           m_fullscreen ? "полноэкранный" : "оконный");
}

void Menu::ToggleRotationMode() {
    m_useQuaternions = !m_useQuaternions;
    m_rotationModeChanged = true;
    
    if (m_useQuaternions) {
        LogSystem("Режим поворота переключен на КВАТЕРНИОНЫ (rx, ry, rz, rw)");
    } else {
        LogSystem("Режим поворота переключен на УГЛЫ ЭЙЛЕРА (rx, ry, rz)");
    }
}

// Callback для автоматической синхронизации состояния полноэкранного режима
void Menu::WindowSizeCallback(GLFWwindow* window, int width, int height) {
    if (window) {
        // Получаем указатель на Menu через user pointer
        Menu* menu = static_cast<Menu*>(glfwGetWindowUserPointer(window));
        if (menu) {
            // Вызываем оригинальный callback для изменения размера
            menu->OnWindowResize(width, height);
            
            // Проверяем текущее состояние GLFW
            GLFWmonitor* monitor = glfwGetWindowMonitor(window);
            bool isFullscreen = (monitor != nullptr);
            
            // Синхронизируем нашу переменную
            if (menu->m_fullscreen != isFullscreen) {
                printf("[Debug] Callback: изменение состояния с %s на %s\n", 
                       menu->m_fullscreen ? "true" : "false", isFullscreen ? "true" : "false");
                menu->m_fullscreen = isFullscreen;
                printf("[System] Автоматическая синхронизация полноэкранного режима: %s\n", 
                       isFullscreen ? "true" : "false");
            }
        }
    }
}

// Статическая функция-обертка для глобального логирования
void Menu::StaticAddLog(void* menuInstance, const std::string& message, const std::string& category, ImVec4 color) {
    if (menuInstance) {
        // Приводим void* к Menu* и вызываем метод AddLog
        static_cast<Menu*>(menuInstance)->AddLog(message, category, color);
    }
}

void Menu::DumpGeometry() {
    if (m_renderer) {
        if (m_renderer->DumpGeometryToFile("geometry.moonstudio")) {
            AddLog("Геометрия успешно дамплена в файл geometry.moonstudio", "System", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        } else {
            AddLog("Ошибка при дампе геометрии", "System", ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        }
    } else {
        AddLog("Ошибка: рендерер не доступен", "System", ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    }
}

