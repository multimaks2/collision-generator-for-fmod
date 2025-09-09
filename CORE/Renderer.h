#pragma once

// Защита от конфликтов Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// Сначала стандартные библиотеки
#include <string>
#include <vector>
#include <algorithm> // Для std::sort

// GLEW ДОЛЖЕН быть первым OpenGL заголовком!
#include "../vendor/glew-2.2.0/include/GL/glew.h"

// Потом Windows API
#ifdef _WIN32
    #include <windows.h>
#endif

// GLFW (после GLEW)
#include <GLFW/glfw3.h>

// ImGui - используем GLEW вместо встроенного loader'а
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Input system
#include "Input.h"

// Menu system
#include "Menu.h"

// Camera system
#include "Camera.h"

// Loader system
#include "Loader.h"

// Collision system
#include "CollisionGtaSaParser.h"



// Windows API для диалога выбора файла
#ifdef _WIN32
    #include <windows.h>
    #include <commdlg.h>
    #include <shlobj.h>
#endif

// Forward declarations больше не нужны - используем Input систему

// Структура для вершины сетки (теперь определена в Grid.h)
// struct GridVertex {
//     float x, y, z;      // Позиция
//     float r, g, b, a;   // Цвет
// };

class Renderer {
public:
    // Структура для DFF модели (должна быть объявлена до использования)
    struct DffModelInstance {
        dff::DffModel model;
        std::string name;
        float x, y, z;
        float rx, ry, rz, rw;
        
        // Современный OpenGL: VBO/VAO вместо Display Lists
        GLuint vao, vbo, ebo;  // Vertex Array Object, Vertex Buffer Object, Element Buffer Object
        GLuint normalVBO;      // VBO для нормалей (если есть)
        bool uploadedToGPU;
        size_t indexCount;      // Количество индексов для рендеринга
        size_t normalCount;     // Количество нормалей
        
        // Конструктор по умолчанию
        DffModelInstance() : vao(0), vbo(0), ebo(0), normalVBO(0), uploadedToGPU(false), indexCount(0), normalCount(0) {}
    };
    

    
    Renderer();
    ~Renderer();
    
    bool Initialize(int width, int height, const char* title);
    void Shutdown();
    bool ShouldClose() const;
    void BeginFrame();
    void EndFrame();
    void Render();
    
    // ImGui методы перенесены в Menu класс
    
    // Методы для 3D сцены
    void CenterCameraOnGrid();
    void Setup3DScene();
    void RenderGrid();
    void RenderCoordinateAxes();
    void RenderAxisLabels();
    void RenderGtaObjects();
    void UpdateCamera();
    void UpdateCameraFromMouse(double deltaX, double deltaY);
    void UpdateFOVFromScroll(double yoffset);
    void UpdateCameraSpeed(float newSpeed);
    
    // Геттеры
    GLFWwindow* GetWindow() const { return m_window; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
    // Геттеры камеры
    float GetCameraFOV() const { return m_camera.GetFOV(); }
    
    // Геттеры статистики
    int GetTotalVertices() const { return m_totalVertices; }
    int GetTotalPolygons() const { return m_totalPolygons; }
    int GetDffVertices() const { return m_dffVertices; }
    int GetDffPolygons() const { return m_dffPolygons; }
    int GetSkyboxVertices() const { return m_skyboxVertices; }
    int GetSkyboxPolygons() const { return m_skyboxPolygons; }
    int GetGtaObjectCount() const { return static_cast<int>(m_gtaObjects.size()); }
    int GetDffModelCount() const { return static_cast<int>(m_dffModels.size()); }
    int GetVisibleGtaObjectCount() const { return static_cast<int>(m_visibleGtaObjects.size()); }
    int GetVisibleDffModelCount() const { return static_cast<int>(m_visibleDffModels.size()); }
    
    // Геттеры/сеттеры настроек рендеринга
    bool IsUsingQuaternions() const { return m_useQuaternions; }
    void SetUseQuaternions(bool use) { m_useQuaternions = use; }
    
    // Радиус рендеринга
    float GetRenderRadius() const { return m_renderRadius; }
    void SetRenderRadius(float radius) { m_renderRadius = radius; }

    
    // Методы для работы с DFF моделями
    void AddDffModel(const dff::DffModel& model, const char* name, float x, float y, float z, float rx, float ry, float rz, float rw);
    void RenderDffModels();
    void LoadAllDffModelsToGPU();
    void LoadVisibleDffModelsToGPU(); // Загружать только видимые модели

    // Методы для работы с IMG архивами
    void SetImgArchives(const std::vector<img::ImgData*>& archives);
    void ClearImgArchives();
    
    // Методы для работы с GTA объектами
    void SetGtaObjects(const std::vector<ipl::IplObject>& objects);
    void AddGtaObject(const ipl::IplObject& object);
    void AddTestObject(int index, int modelId, const char* name, float x, float y, float z, float rx, float ry, float rz, float rw);
    
    // Методы для получения видимых объектов
    const std::vector<DffModelInstance>& GetVisibleDffModels() const;
    const std::vector<ipl::IplObject>& GetVisibleGtaObjects() const;
    
    // Методы для получения всех объектов
    const std::vector<DffModelInstance>& GetAllDffModels() const { return m_dffModels; }
    
    // Методы для статистики
    void ResetRenderStats();
    void UpdateRenderStats();
    void ForceUpdateVisibleObjects(); // Принудительно обновить видимые объекты
    
    // Метод для дампа геометрии в файл
    bool DumpGeometryToFile(const std::string& filename = "geometry.moonstudio");
    
    // Проверка ImGui состояния
    bool IsImGuiHovered() const;
    
private:
    // Основные переменные
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    bool m_initialized;
    
    // 3D сцена
    bool m_3dSceneInitialized;
    
    // Камера управляется через Camera класс
    
    // Настройки скорости
    float m_baseSpeed;        // Базовая скорость
    float m_speedMultiplier;  // Множитель скорости
    
    // Параметры сетки (используются в Grid классе)
    int m_gridSize;
    int m_gridSpacing;
    int m_gridSegments;
    
    // Input system
    Input m_input;
    
    // Menu system
    Menu m_menu;
    
    // Camera system
    Camera m_camera;
    
    // GTA объекты
    std::vector<ipl::IplObject> m_gtaObjects;
    
    // DFF модели
    std::vector<DffModelInstance> m_dffModels;
    

    // IMG архивы для извлечения моделей
    std::vector<img::ImgData*> m_imgArchives;
    
    // Кэш видимых объектов (обновляется каждый кадр)
    mutable std::vector<DffModelInstance> m_visibleDffModels;
    mutable std::vector<ipl::IplObject> m_visibleGtaObjects;
    
    // Кэш позиции камеры для оптимизации фильтрации
    mutable float m_lastCameraX, m_lastCameraY;
    mutable bool m_cameraMoved;
    
    // Статистика рендеринга
    int m_totalVertices;
    int m_totalPolygons;
    int m_dffVertices;
    int m_dffPolygons;
    int m_skyboxVertices;
    int m_skyboxPolygons;
    
    // Настройки рендеринга
    bool m_useQuaternions;
    
    // Радиус рендеринга
    float m_renderRadius;

    
    // Отладочные флаги
    bool m_debugMode;
    
    // Профилирование производительности
    double m_lastFrameTime;
    double m_uploadTime;
    double m_renderTime;
    
    // OpenGL методы
    void SetupOpenGL();
    void SetupCallbacks();
    
    // Вспомогательные методы
    void ProcessInput();
    
    // Методы фильтрации объектов
    bool IsObjectInRenderRadius(const DffModelInstance& obj) const;
    bool IsObjectInRenderRadius(const ipl::IplObject& obj) const;
    
    // Методы для современного OpenGL (VBO/VAO)
    bool UploadModelToGPU(DffModelInstance& instance);
    void DeleteModelFromGPU(DffModelInstance& instance);
    void CleanupAllGPUModels();
    

    
    // Fallback методы для проблемных моделей
    dff::DffModel LoadDffFromUnpack(const std::string& modelName);
    bool ExtractModelFromImg(const std::string& modelName, const std::string& outputPath);
    
    // --- Grid (modern OpenGL) ---
    bool CreateGridResources();
    void DestroyGridResources();
    bool CreateAxesResources();
    void DestroyAxesResources();
    bool CreateColShader();
    void DestroyColShader();

    // Grid resources
    GLuint m_gridVAO = 0;
    GLuint m_gridVBO = 0;
    GLuint m_gridShader = 0;
    GLsizei m_gridVertexCount = 0; // number of vertices for GL_LINES
    GLuint m_axesVAO = 0;
    GLuint m_axesVBO = 0;
    GLsizei m_axesVertexCount = 0; // 6 vertices (3 lines)
    GLuint m_modelShader = 0; // шейдер для DFF моделей с освещением
    GLuint m_colShader = 0;   // шейдер для COL моделей (коллизионная геометрия)

    // --- Skybox resources ---
    GLuint m_skyboxVAO = 0;
    GLuint m_skyboxVBO = 0;
    GLuint m_skyboxShader = 0;
    GLuint m_skyboxTexture = 0; // Текстура для скайбокса
    bool m_skyboxInitialized = false;

    // Shader helpers
    GLuint CompileShader(GLenum type, const char* src);
    GLuint LinkProgram(GLuint vs, GLuint fs);

    // Skybox methods
    bool CreateSkyboxResources();
    void DestroySkyboxResources();
    void RenderSkybox();
    bool LoadSkyboxTextures();

};
