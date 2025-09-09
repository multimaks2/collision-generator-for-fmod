#pragma once

// GLEW ДОЛЖЕН быть первым OpenGL заголовком!
#include "../vendor/glew-2.2.0/include/GL/glew.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <mutex>
#include "Logger.h"

// Forward declarations
class Renderer;

class Menu {
public:
    Menu();
    ~Menu();
    
    // Инициализация
    void Initialize(GLFWwindow* window);
    void Shutdown();
    
    // Основные методы
    void BeginFrame();
    void Render();
    void EndFrame();
    
    // Установка ссылок на другие системы
    void SetRenderer(Renderer* renderer);
    
    // Методы для получения информации
    void SetCameraInfo(float x, float y, float z, float rotX, float rotY, float fov);
    void SetCameraSpeed(float baseSpeed);
    
    // Callbacks для настроек
    void OnCameraSpeedChanged(float newSpeed);
    void OnRenderRadiusChanged(float newRadius);
    
    // Getters для настроек
    bool IsCameraSpeedChanged() const { return m_cameraSpeedChanged; }
    float GetNewCameraSpeed() const { return m_newCameraSpeed; }
    bool IsRenderRadiusChanged() const { return m_renderRadiusChanged; }
    float GetNewRenderRadius() const { return m_newRenderRadius; }
    
    // Геттеры для режима поворота
    bool IsRotationModeChanged() const { return m_rotationModeChanged; }
    bool GetUseQuaternions() const { return m_useQuaternions; }
    
    // Сброс флагов
    void ResetRequestFlags();
    
    // Обработчик изменения размера окна
    void OnWindowResize(int width, int height);
    
    // Методы логирования (публичные для доступа из Logger)
    void AddLog(const std::string& message, const std::string& category = "General", ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    void ClearLogs();
    void ExportLogs();
    void CopyLogsToClipboard(const std::string& category = "");
    
    // FPS методы
    void UpdateFPS();
    
    // Полноэкранный режим
    void ToggleFullscreen();
    
    // Переключение режима поворота
    void ToggleRotationMode();
    
    // Дамп геометрии
    void DumpGeometry();
    
    // Callback для синхронизации состояния полноэкранного режима
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);
    
    // Статическая функция-обертка для глобального логирования
    static void StaticAddLog(void* menuInstance, const std::string& message, const std::string& category, ImVec4 color);
    
private:
    // ImGui методы
    void SetupImGui();
    void RenderPerformanceMonitor();
    
    // Стилизация
    void SetupStyle();
    
private:
    // Ссылки на системы
    GLFWwindow* m_window;
    Renderer* m_renderer;
    
    // Состояние ImGui
    bool m_initialized;
    bool m_fontLoaded;  // Поддержка кириллицы
    ImFont* m_smallFont;  // Маленький шрифт для консоли логов
    
    // Информация о камере
    float m_cameraX, m_cameraY, m_cameraZ;
    float m_cameraRotX, m_cameraRotY;
    float m_fov;
    
    // Информация о производительности
    float m_cameraSpeed;
    
    // Флаги для настроек
    bool m_cameraSpeedChanged;
    float m_newCameraSpeed;
    bool m_renderRadiusChanged;
    float m_newRenderRadius;
    
    // Размеры окна
    int m_windowWidth;
    int m_windowHeight;
    
    // Пропорции ImGui окна (в процентах от главного окна)
    float m_imguiWidthPercent;
    float m_imguiHeightPercent;
    
    // Настройки отображения HUD
    bool m_showMainOverlay;
    bool m_showStatsBar;
    bool m_showCameraInfo;
    
    // Система логирования
    struct LogEntry {
        std::string message;
        std::string category;
        ImVec4 color;
    };
    
    std::vector<LogEntry> m_logs;
    std::mutex m_logMutex;
    bool m_autoScroll;
    int m_maxLogEntries;
    
    // FPS система
    double m_lastFrameTime;
    double m_frameTime;
    double m_avgFrameTime;
    int m_frameCount;
    static const int FPS_SAMPLE_COUNT = 60;
    
    // Полноэкранный режим
    bool m_fullscreen;
    int m_windowedWidth;
    int m_windowedHeight;
    int m_windowedX;
    int m_windowedY;
    
    // Режим поворота объектов
    bool m_useQuaternions;
    bool m_rotationModeChanged;
};
