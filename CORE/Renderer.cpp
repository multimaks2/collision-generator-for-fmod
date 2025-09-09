#include "Renderer.h"
#include "Logger.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cstdio>

// Windows API
#ifdef _WIN32
    #include <commdlg.h>
    #include <shlobj.h>
#endif

// GLEW уже включен в Renderer.h
#include <Loader.h>

// glm для матриц
#include "../vendor/glm-master/glm/glm.hpp"
#include "../vendor/glm-master/glm/gtc/matrix_transform.hpp"
#include "../vendor/glm-master/glm/gtc/type_ptr.hpp"

// Больше не используется (fixed-function). Матрицы считаем через glm
static glm::mat4 BuildPerspective(float fovyDeg, float aspect, float zNear, float zFar) {
    return glm::perspective(glm::radians(fovyDeg), aspect, zNear, zFar);
}

// Построение матрицы из кватерниона (rx,ry,rz,rw)
static glm::mat4 BuildRotationFromQuat(float rx, float ry, float rz, float rw) {
    const float len = sqrtf(rx*rx + ry*ry + rz*rz + rw*rw);
    if (len > 0.0001f) {
        rx /= len; ry /= len; rz /= len; rw /= len;
    }
    // Правильный порядок: (w, x, y, z) для GLM
    const glm::quat q(rw, rx, ry, rz);
    return glm::mat4_cast(q);
}

// Compatibility helper for legacy fixed-function paths still present below
void applyQuaternion(float rx, float ry, float rz, float rw) {
    float length = sqrtf(rx * rx + ry * ry + rz * rz + rw * rw);
    if (length > 0.0001f) {
        rx /= length; ry /= length; rz /= length; rw /= length;
    }
    float m[16] = {
        1.0f - 2.0f * (ry * ry + rz * rz),  2.0f * (rx * ry - rw * rz),      2.0f * (rx * rz + rw * ry),      0.0f,
        2.0f * (rx * ry + rw * rz),        1.0f - 2.0f * (rx * rx + rz * rz), 2.0f * (ry * rz - rw * rx),      0.0f,
        2.0f * (rx * rz - rw * ry),        2.0f * (ry * rz + rw * rx),      1.0f - 2.0f * (rx * rx + ry * ry), 0.0f,
        0.0f,                               0.0f,                             0.0f,                             1.0f
    };
    glMultMatrixf(m);
}

Renderer::Renderer() : m_window(nullptr), m_width(0), m_height(0), m_initialized(false),
    m_3dSceneInitialized(false), 
    m_baseSpeed(1000.0f), m_speedMultiplier(1.0f), // Базовая скорость: 1000, максимум: 3000
    m_gridSize(3000), m_gridSpacing(100), m_gridSegments(30),
    m_totalVertices(0), m_totalPolygons(0), m_dffVertices(0), m_dffPolygons(0), m_skyboxVertices(0), m_skyboxPolygons(0),
        m_useQuaternions(true), // По умолчанию включаем кватернионы
    m_renderRadius(1500.0f), // По умолчанию радиус 1500 единиц
    m_debugMode(false), // Отладочный режим выключен по умолчанию
    m_lastFrameTime(0.0), m_uploadTime(0.0), m_renderTime(0.0), // Профилирование
    m_visibleDffModels(), m_visibleGtaObjects(), // Инициализируем пустые векторы
    m_lastCameraX(-999999.0f), m_lastCameraY(-999999.0f), m_cameraMoved(false) { // Инициализируем кэш камеры
    
}

Renderer::~Renderer() {
    // Очищаем все Display Lists перед завершением
    CleanupAllGPUModels();

    DestroyGridResources();
    DestroyAxesResources();
    DestroySkyboxResources();
    
    Shutdown();
}

bool Renderer::Initialize(int width, int height, const char* title) {
    m_width = width;
    m_height = height;
    
    // Инициализируем GLFW
    if (!glfwInit()) {
        return false;
    }

    // Настраиваем OpenGL 3.3 Core Profile для современного рендеринга
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        return false;
    }
    

    glfwMakeContextCurrent(m_window);
    
    // Настраиваем callbacks
    SetupCallbacks();

    SetupOpenGL();
    
    // Инициализируем GLEW для современного OpenGL
    //printf("[Renderer] Инициализация GLEW...\n");
    //printf("[Renderer] GLEW версия: %s\n", glewGetString(GLEW_VERSION));
    
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        //printf("[Renderer] Ошибка инициализации GLEW: %s\n", glewGetErrorString(err));
        return false;
    }
    
    //printf("[Renderer] GLEW успешно инициализирован\n");
    //printf("[Renderer] OpenGL версия: %s\n", glGetString(GL_VERSION));
    //printf("[Renderer] OpenGL vendor: %s\n", glGetString(GL_VENDOR));
    //printf("[Renderer] OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    
    // Проверяем, что GLEW загрузил базовые типы OpenGL
    //printf("[Renderer] Проверка базовых типов GLEW...\n");
    //printf("[Renderer] GLchar определен: %s\n", (sizeof(GLchar) > 0) ? "ДА" : "НЕТ");
    //printf("[Renderer] GLint определен: %s\n", (sizeof(GLint) > 0) ? "ДА" : "НЕТ");
    //printf("[Renderer] GLuint определен: %s\n", (sizeof(GLuint) > 0) ? "ДА" : "НЕТ");
    //printf("[Renderer] GLfloat определен: %s\n", (sizeof(GLfloat) > 0) ? "ДА" : "НЕТ");
    
    // Проверяем поддержку OpenGL 3.3
    if (!GLEW_VERSION_3_3) {
        //printf("[Renderer] Ошибка: OpenGL 3.3 не поддерживается\n");
        //printf("[Renderer] Доступная версия: %s\n", glGetString(GL_VERSION));
        return false;
    }
    
    //printf("[Renderer] OpenGL 3.3 Core Profile поддерживается\n");
    
    // Включаем V-Sync для плавного движения камеры
    glfwSwapInterval(1);
    
    // Инициализация Camera системы
    m_camera.Initialize(0.0f, -15.0f, 10.0f, 0.0f, 0.0f, 90.0f);
    m_camera.SetMouseSensitivity(0.1f);
    m_camera.SetRotationLimits(0, -135); // Ограничения поворота вверх/вниз
    m_camera.SetFOVLimits(1.0f, 160.0f); // Минимальный FOV: 1°, Максимальный FOV: 160°

    // Создаем ресурсы сетки и осей (современный OpenGL)
    CreateGridResources();
    CreateAxesResources();
    
    // Создаем ресурсы скайбокса
    CreateSkyboxResources();
    
    // Настраиваем 3D сцену ПЕРЕД инициализации ImGui
    Setup3DScene();
    
    // Инициализация Menu системы ПОСЛЕ настройки OpenGL
    m_menu.Initialize(m_window);
    m_menu.SetRenderer(this);

    //GtaDatData gtaData;
    //if (dat::loadGtaDat("data\\gta.dat", gtaData))
    //{
    //    auto iplEntries = gtaData.getEntriesByType(DataType::IPL);
    //    auto imgEntries = gtaData.getEntriesByType(DataType::IMG);
    //    //printf("IPL %s", iplEntries);
    //    //printf("IMG %s", imgEntries);
    //}

    m_initialized = true;
    return true;
}
// --- Grid & shader helpers ---
GLuint Renderer::CompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {

    }
    return s;
}

GLuint Renderer::LinkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {

    }
    glDetachShader(p, vs); glDetachShader(p, fs);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

bool Renderer::CreateGridResources() {
    if (m_gridVAO) return true;
    // Build simple grid lines on Z=0
    const float gridSize = 3000.0f;
    const float step = 100.0f;
    std::vector<float> verts;
    for (float x=-gridSize; x<=gridSize+0.1f; x+=step) {
        verts.push_back(x); verts.push_back(-gridSize); verts.push_back(0.0f);
        verts.push_back(x); verts.push_back( gridSize); verts.push_back(0.0f);
    }
    for (float y=-gridSize; y<=gridSize+0.1f; y+=step) {
        verts.push_back(-gridSize); verts.push_back(y); verts.push_back(0.0f);
        verts.push_back( gridSize); verts.push_back(y); verts.push_back(0.0f);
    }
    m_gridVertexCount = (GLsizei)(verts.size()/3);

    glGenVertexArrays(1, &m_gridVAO);
    glBindVertexArray(m_gridVAO);
    glGenBuffers(1, &m_gridVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glBindVertexArray(0);

    // Простой шейдер для сетки и осей (без освещения)
    static const char* vsSrc =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "uniform mat4 uMVP;\n"
        "void main() {\n"
        "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
        "}\n";
    static const char* fsSrc =
        "#version 330 core\n"
        "uniform vec4 uColor;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = uColor;\n"
        "}\n";
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
    m_gridShader = LinkProgram(vs, fs);
    
    // Создаем шейдер для DFF моделей с освещением
    static const char* modelVsSrc =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "layout(location=1) in vec3 aNormal;\n"
        "uniform mat4 uMVP;\n"
        "uniform mat4 uModel;\n"
        "out vec3 FragPos;\n"
        "out vec3 Normal;\n"
        "void main() {\n"
        "    FragPos = vec3(uModel * vec4(aPos, 1.0));\n"
        "    Normal = mat3(transpose(inverse(uModel))) * aNormal;\n"
        "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
        "}\n";
    static const char* modelFsSrc =
        "#version 330 core\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "uniform vec3 uLightPos;\n"
        "uniform vec3 uViewPos;\n"
        "uniform vec3 uLightAmbient;\n"
        "uniform vec3 uLightDiffuse;\n"
        "uniform vec3 uLightSpecular;\n"
        "uniform vec3 uMaterialAmbient;\n"
        "uniform vec3 uMaterialDiffuse;\n"
        "uniform vec3 uMaterialSpecular;\n"
        "uniform float uMaterialShininess;\n"
        "uniform int uPolygonCount;\n"
        "uniform int uMaxPolygons;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    // Вычисляем цвет на основе количества полигонов\n"
        "    float ratio = float(uPolygonCount) / float(max(uMaxPolygons, 1));\n"
        "    vec3 modelColor;\n"
        "    if (ratio < 0.5) {\n"
        "        // От зеленого к желтому (мало полигонов)\n"
        "        float t = ratio * 2.0;\n"
        "        modelColor = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), t);\n"
        "    } else {\n"
        "        // От желтого к красному (много полигонов)\n"
        "        float t = (ratio - 0.5) * 2.0;\n"
        "        modelColor = mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), t);\n"
        "    }\n"
        "    \n"
        "    // Нормализуем нормаль\n"
        "    vec3 norm = normalize(Normal);\n"
        "    \n"
        "    // Основной источник света\n"
        "    vec3 lightDir = normalize(uLightPos - FragPos);\n"
        "    \n"
        "    // Ambient\n"
        "    vec3 ambient = uLightAmbient * uMaterialAmbient;\n"
        "    \n"
        "    // Diffuse\n"
        "    float diff = max(dot(norm, lightDir), 0.0);\n"
        "    vec3 diffuse = uLightDiffuse * (diff * uMaterialDiffuse);\n"
        "    \n"
        "    // Specular\n"
        "    vec3 viewDir = normalize(uViewPos - FragPos);\n"
        "    vec3 reflectDir = reflect(-lightDir, norm);\n"
        "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterialShininess);\n"
        "    vec3 specular = uLightSpecular * (spec * uMaterialSpecular);\n"
        "    \n"
        "    // Комбинируем освещение\n"
        "    vec3 result = ambient + diffuse + specular;\n"
        "    \n"
        "    FragColor = vec4(result * modelColor, 1.0);\n"
        "}\n";
    
    GLuint modelVs = CompileShader(GL_VERTEX_SHADER, modelVsSrc);
    GLuint modelFs = CompileShader(GL_FRAGMENT_SHADER, modelFsSrc);
    m_modelShader = LinkProgram(modelVs, modelFs);
    
    return m_gridVAO!=0 && m_gridShader!=0 && m_modelShader!=0;
}

void Renderer::DestroyGridResources() {
    if (m_gridVBO) { glDeleteBuffers(1,&m_gridVBO); m_gridVBO=0; }
    if (m_gridVAO) { glDeleteVertexArrays(1,&m_gridVAO); m_gridVAO=0; }
    if (m_gridShader) { glDeleteProgram(m_gridShader); m_gridShader=0; }
    if (m_modelShader) { glDeleteProgram(m_modelShader); m_modelShader=0; }
    m_gridVertexCount = 0;
}

bool Renderer::CreateAxesResources() {
    if (m_axesVAO) return true;
    // 3 axis lines from origin: X (red), Y (green), Z (blue)
    const float L = 500.0f;
    const float verts[] = {
        // X axis
        0.0f, 0.0f, 0.0f,  L, 0.0f, 0.0f,
        // Y axis
        0.0f, 0.0f, 0.0f,  0.0f, L, 0.0f,
        // Z axis
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, L
    };
    m_axesVertexCount = 6;
    glGenVertexArrays(1, &m_axesVAO);
    glBindVertexArray(m_axesVAO);
    glGenBuffers(1, &m_axesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_axesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glBindVertexArray(0);
    return true;
}

void Renderer::DestroyAxesResources() {
    if (m_axesVBO) { glDeleteBuffers(1,&m_axesVBO); m_axesVBO=0; }
    if (m_axesVAO) { glDeleteVertexArrays(1,&m_axesVAO); m_axesVAO=0; }
    m_axesVertexCount = 0;
}

void Renderer::SetupOpenGL() {
    // Устанавливаем viewport
    glViewport(0, 0, m_width, m_height);
    
    // Настройки состояния для Core Profile
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Убираем устаревшие OpenGL 1.1 вызовы - они не работают в Core Profile
    // Вместо этого используем современные шейдеры с uniform переменными
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    //LogRender("SetupOpenGL: современный OpenGL 3.3 Core Profile настроен");
}

void Renderer::SetupCallbacks() {
    // Инициализируем Input систему вместо прямых callback'ов
    m_input.Initialize(m_window, this);
}

void Renderer::Shutdown() {
    // Очищаем Menu систему
    m_menu.Shutdown();
    DestroyGridResources();
    DestroyAxesResources();
    
    // Очищаем Input систему
    m_input.Shutdown();
    
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_initialized) {
        glfwTerminate();
        m_initialized = false;
    }
}

bool Renderer::ShouldClose() const {
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

void Renderer::BeginFrame() {
    if (!m_initialized) return;
    
    // Очищаем буферы
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Обновляем FOV и движение с инертностью (используем фиксированное время)
    m_camera.UpdateFOVInertia();
    m_camera.UpdateMovementInertia();
    
    // Начинаем Menu кадр
    m_menu.BeginFrame();
}

void Renderer::EndFrame() {
    if (!m_initialized) return;
    
    // Завершаем Menu кадр
    m_menu.EndFrame();
    
    // Меняем буферы
    glfwSwapBuffers(m_window);
    
    // Обрабатываем события
    glfwPollEvents();
}

void Renderer::Render() {
    if (!m_initialized || !m_window) return;
    
    // Загружаем DFF модели в GPU в первом кадре
    static bool firstFrame = true;
    if (firstFrame) {
        //printf("[Renderer] Render: первый кадр, загружаем DFF модели в GPU...\n");
        LoadAllDffModelsToGPU();
        firstFrame = false;
    }
    
    // Обрабатываем ввод
    ProcessInput();
    
    // Матрицы в Core Profile: считаем через glm
    const glm::mat4 proj = BuildPerspective(m_camera.GetFOV(), (float)m_width / (float)m_height, 0.1f, 10000.0f);
    glm::mat4 view(1.0f);
    // Простейшая view: вращение и перенос в обратную сторону значений камеры
    view = glm::rotate(view, glm::radians(m_camera.GetRotationX()), glm::vec3(1,0,0));
    view = glm::rotate(view, glm::radians(m_camera.GetRotationY()), glm::vec3(0,0,1));
    view = glm::translate(view, glm::vec3(-m_camera.GetX(), -m_camera.GetY(), -m_camera.GetZ()));
    
    // Рендерим 3D сцену
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Сбрасываем статистику рендеринга в начале каждого кадра
    ResetRenderStats();
    
    // Рендерим скайбокс (должен быть первым, чтобы заполнить фон)
    RenderSkybox();
    
    // Настраиваем глубину
    glEnable(GL_DEPTH_TEST);
    
    // Рендерим сетку (современный OpenGL)
    if (m_gridShader && m_gridVAO) {
        glUseProgram(m_gridShader);
        const glm::mat4 mvp = proj * view; // сетка в мировых координатах на Z=0
        const glm::mat4 model(1.0f); // единичная матрица для сетки
        
        const GLint locMVP = glGetUniformLocation(m_gridShader, "uMVP");
        const GLint colLoc = glGetUniformLocation(m_gridShader, "uColor");
        
        if (locMVP >= 0) glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        if (colLoc >= 0) glUniform4f(colLoc, 0.6f, 0.6f, 0.6f, 0.7f);
        
        glBindVertexArray(m_gridVAO);
        glDrawArrays(GL_LINES, 0, m_gridVertexCount);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    // Рисуем оси X/Y/Z разными цветами
    if (m_gridShader && m_axesVAO) {
        glUseProgram(m_gridShader);
        const glm::mat4 mvp = proj * view;
        const glm::mat4 model(1.0f);
        
        const GLint locMVP = glGetUniformLocation(m_gridShader, "uMVP");
        const GLint colLoc = glGetUniformLocation(m_gridShader, "uColor");
        
        if (locMVP >= 0) glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        
        glBindVertexArray(m_axesVAO);
        // X - красный
        if (colLoc >= 0) glUniform4f(colLoc, 1.0f, 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINES, 0, 2);
        // Y - зеленый
        if (colLoc >= 0) glUniform4f(colLoc, 0.0f, 1.0f, 0.0f, 1.0f);
        glDrawArrays(GL_LINES, 2, 2);
        // Z - синий
        if (colLoc >= 0) glUniform4f(colLoc, 0.0f, 0.0f, 1.0f, 1.0f);
        glDrawArrays(GL_LINES, 4, 2);
        glBindVertexArray(0);
        glUseProgram(0);
    }
    
    // Рендерим оси координат (теперь уже включены в RenderGrid)
    // RenderCoordinateAxes();
    RenderAxisLabels();
    
    // Рендерим GTA объекты в виде сфер
    RenderGtaObjects();
    
    // Рендерим DFF модели
    RenderDffModels();
    

    // Обновляем статистику рендеринга каждый кадр для корректного отображения в ImGui
    UpdateRenderStats();
    
    // Рендерим Menu поверх 3D сцены
    m_menu.Render();
    
    // Применяем ограничение FPS если установлено
    // if (m_targetFPS > 0) {
    //     static double lastFrameTime = 0.0;
    //     double currentTime = glfwGetTime();
    //     double targetFrameTime = 1.0 / m_targetFPS;
        
    //     if (currentTime - lastFrameTime < targetFrameTime) {
    //         double sleepTime = targetFrameTime - (currentTime - lastFrameTime);
    //             // Простое ограничение FPS через задержку
    //             while (glfwGetTime() - currentTime < sleepTime) {
    //                 // Ждем
    //             }
    //         }
    //         lastFrameTime = glfwGetTime();
    //     }
    // }
}

// ImGui методы перенесены в Menu класс

// Методы для 3D сцены
void Renderer::CenterCameraOnGrid() {
    // Сбрасываем камеру в начальное положение
    m_camera.Reset();
}

void Renderer::Setup3DScene() {
    if (m_3dSceneInitialized) return;
    
    // Сетка теперь рендерится напрямую в RenderGrid
    // Убираем старую сетку - используем только новую
    // GenerateGrid();  // Убрано
    // GenerateAxis();  // Убрано
    
    // Центрируем камеру на сетке
    CenterCameraOnGrid();
    
    m_3dSceneInitialized = true;
}

// Методы для рендеринга осей координат
void Renderer::RenderGrid() {}

void Renderer::RenderCoordinateAxes() {
    if (!m_3dSceneInitialized) return;
    
    // В OpenGL 3.3 Core Profile используем современный подход
    // Пока что оставляем заглушку - оси координат будут переписаны на VBO/VAO
    // TODO: Переписать на современный OpenGL с использованием VBO/VAO для осей координат
}

void Renderer::RenderAxisLabels() {
    if (!m_3dSceneInitialized) return;
    
    // Пока что просто рендерим подписи используя ImGui
    // в будущем можно будет добавить 3D текст если понадобится
    
    // Рендерим подписи в ImGui окне
    // Это проще и надежнее чем работа с 3D текстом в OpenGL 1.1
}

void Renderer::UpdateCamera() {
    // Применяем трансформации камеры через Camera класс
    m_camera.ApplyTransform();
}

void Renderer::ProcessInput() {
    if (!m_window || !m_initialized) return;
    
    // Обновляем Input систему
    m_input.Update();
    
    // Обновляем информацию в Menu
    m_menu.SetCameraInfo(m_camera.GetX(), m_camera.GetY(), m_camera.GetZ(), 
                         m_camera.GetRotationX(), m_camera.GetRotationY(), m_camera.GetFOV());
    m_menu.SetCameraSpeed(m_baseSpeed);
    
    // Базовые скорости движения
    float currentSpeed = m_baseSpeed * m_speedMultiplier;
    
    // Shift для ускорения (200%), Alt для замедления (50%)
    if (m_input.IsKeyPressed(GLFW_KEY_LEFT_SHIFT) || m_input.IsKeyPressed(GLFW_KEY_RIGHT_SHIFT)) {
        currentSpeed = m_baseSpeed * 2.0f; // Ускорение 200%
    }
    if (m_input.IsKeyPressed(GLFW_KEY_LEFT_ALT) || m_input.IsKeyPressed(GLFW_KEY_RIGHT_ALT)) {
        currentSpeed = m_baseSpeed * 0.5f; // Замедление 50%
    }
    
    // Ограничиваем максимальную скорость до 3000
    currentSpeed = std::min(currentSpeed, 3000.0f);
    
    // WASD для движения камеры с учетом направления
    if (m_input.IsKeyPressed(GLFW_KEY_W)) {
        // Движение вперед по направлению камеры
        m_camera.MoveForward(currentSpeed * 0.016f);
    }
    if (m_input.IsKeyPressed(GLFW_KEY_S)) {
        // Движение назад против направления камеры
        m_camera.MoveForward(-currentSpeed * 0.016f);
    }
    if (m_input.IsKeyPressed(GLFW_KEY_A)) {
        // Движение влево перпендикулярно направлению камеры
        m_camera.MoveRight(-currentSpeed * 0.016f);
    }
    if (m_input.IsKeyPressed(GLFW_KEY_D)) {
        // Движение вправо перпендикулярно направлению камеры
        m_camera.MoveRight(currentSpeed * 0.016f);
    }
    
    // Пробел для подъема, Control для спуска
    if (m_input.IsKeyPressed(GLFW_KEY_SPACE)) {
        m_camera.MoveUp(currentSpeed * 0.016f);
    }
    if (m_input.IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || m_input.IsKeyPressed(GLFW_KEY_RIGHT_CONTROL)) {
        m_camera.MoveUp(-currentSpeed * 0.016f);
    }
    
    // ESC для выхода
    if (m_input.IsKeyPressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
    
    // R для центрирования камеры на сетке
    if (m_input.IsKeyJustPressed(GLFW_KEY_R)) {
        CenterCameraOnGrid();
    }
    




    
    // Обрабатываем callback'и настроек
    if (m_menu.IsCameraSpeedChanged()) {
        UpdateCameraSpeed(m_menu.GetNewCameraSpeed());
    }
    
    // Обрабатываем изменения радиуса рендеринга
    if (m_menu.IsRenderRadiusChanged()) {
        float oldRadius = m_renderRadius;
        SetRenderRadius(m_menu.GetNewRenderRadius());
        LogRender("Радиус рендеринга изменен: " + std::to_string(oldRadius) + " -> " + std::to_string(m_renderRadius));
    }
    
    // Обрабатываем изменения режима поворота
    if (m_menu.IsRotationModeChanged()) {
        SetUseQuaternions(m_menu.GetUseQuaternions());
    }
    
    // Сбрасываем флаги Menu
    m_menu.ResetRequestFlags();
}

// Обновление камеры от движения мыши (как в HTML версии)
void Renderer::UpdateCameraFromMouse(double deltaX, double deltaY) {
    // Применяем вращение камеры при движении мыши через Camera класс
    m_camera.Rotate(static_cast<float>(deltaX), static_cast<float>(deltaY));
}

// Обновление FOV от колеса мыши
void Renderer::UpdateFOVFromScroll(double yoffset) {
    // Изменяем FOV на основе прокрутки колеса мыши через Camera класс
    m_camera.UpdateFOV(static_cast<float>(yoffset));
    
    // Изменяем чувствительность мыши в зависимости от FOV
    float currentFOV = m_camera.GetFOV();
    
    // При уменьшении FOV (зум) уменьшаем чувствительность мыши
    // При увеличении FOV (широкий угол) увеличиваем чувствительность
    float fovRatio = currentFOV / 90.0f; // 90.0f как базовая точка
    
    // Ограничиваем чувствительность в разумных пределах
    float newSensitivity = 0.1f * fovRatio;
    newSensitivity = std::max(0.01f, std::min(0.5f, newSensitivity)); // Минимум 0.01, максимум 0.5
    
    m_camera.SetMouseSensitivity(newSensitivity);
}

// Обновление скорости камеры
void Renderer::UpdateCameraSpeed(float newSpeed) {
    // Ограничиваем максимальную скорость до 3000
    m_baseSpeed = std::min(newSpeed, 3000.0f);
    // Теперь скорость управляется через m_baseSpeed
}


bool Renderer::IsImGuiHovered() const {
    // Проверяем, находится ли курсор над любым ImGui окном или элементом
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || 
           ImGui::IsAnyItemHovered() ||
           ImGui::GetIO().WantCaptureMouse;
}

// Метод для установки GTA объектов
void Renderer::SetGtaObjects(const std::vector<ipl::IplObject>& objects) {
    m_gtaObjects = objects;
    //printf("[Renderer] Установлено %zu GTA объектов для отрисовки\n", m_gtaObjects.size());
}

// Метод для добавления одного GTA объекта
void Renderer::AddGtaObject(const ipl::IplObject& object) {
    m_gtaObjects.push_back(object);
    //printf("[Renderer] Добавлен объект: ID: %d, Имя: %s, Позиция: (%.2f, %.2f, %.2f), Поворот: (%.2f, %.2f, %.2f, %.2f)\n", 
           //object.modelId, object.name.c_str(), object.x, object.y, object.z, object.rx, object.ry, object.rz, object.rw);
}

// Метод для добавления тестового объекта с отдельными параметрами
void Renderer::AddTestObject(int index, int modelId, const char* name, float x, float y, float z, float rx, float ry, float rz, float rw) {
    // Создаем объект с переданными параметрами
    ipl::IplObject testObject(modelId, name, 0, x, y, z, rx, ry, rz, rw, 0);
    
    // Добавляем в вектор объектов
    m_gtaObjects.push_back(testObject);
    
    // Выводим информацию о кватернионе
    float quatLength = sqrt(rx * rx + ry * ry + rz * rz + rw * rw);
    //LogRender("Добавлен тестовый объект #" + std::to_string(index) + ": ID: " + std::to_string(modelId) + ", Имя: " + name);
    //LogRender("  Позиция: (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
    //LogRender("  Кватернион: (" + std::to_string(rx) + ", " + std::to_string(ry) + ", " + std::to_string(rz) + ", " + std::to_string(rw) + ") [длина: " + std::to_string(quatLength) + "]");
    
    if (quatLength > 0.001f) {
        // Нормализованный кватернион
        float nx = rx / quatLength, ny = ry / quatLength, nz = rz / quatLength, nw = rw / quatLength;
        //LogRender("  Нормализованный: (" + std::to_string(nx) + ", " + std::to_string(ny) + ", " + std::to_string(nz) + ", " + std::to_string(nw) + ")");
    }
}

// Метод для отрисовки GTA объектов в виде кубов
void Renderer::RenderGtaObjects() {
    // Получаем только видимые GTA объекты
    const std::vector<ipl::IplObject>& visibleObjects = GetVisibleGtaObjects();

    if (visibleObjects.empty()) {
        return; // Нет видимых объектов
    }

    // Используем тот же простой шейдер, что и для сетки
    if (!m_gridShader) return;
    glUseProgram(m_gridShader);

    // Матрицы
    const glm::mat4 proj = BuildPerspective(m_camera.GetFOV(), (float)m_width / (float)m_height, 0.1f, 10000.0f);
    glm::mat4 view(1.0f);
    view = glm::rotate(view, glm::radians(m_camera.GetRotationX()), glm::vec3(1,0,0));
    view = glm::rotate(view, glm::radians(m_camera.GetRotationY()), glm::vec3(0,0,1));
    view = glm::translate(view, glm::vec3(-m_camera.GetX(), -m_camera.GetY(), -m_camera.GetZ()));

    const GLint locMVP = glGetUniformLocation(m_gridShader, "uMVP");
    const GLint locCol = glGetUniformLocation(m_gridShader, "uColor");

    // Создаем простой куб VBO/VAO если еще не создан
    static GLuint cubeVAO = 0, cubeVBO = 0, cubeEBO = 0;
    static bool cubeInitialized = false;
    
    if (!cubeInitialized) {
        // Простой куб: 8 вершин, 12 треугольников
        const float vertices[] = {
            // позиции (x, y, z)
            -1.0f, -1.0f, -1.0f,  // 0
             1.0f, -1.0f, -1.0f,  // 1
             1.0f,  1.0f, -1.0f,  // 2
            -1.0f,  1.0f, -1.0f,  // 3
            -1.0f, -1.0f,  1.0f,  // 4
             1.0f, -1.0f,  1.0f,  // 5
             1.0f,  1.0f,  1.0f,  // 6
            -1.0f,  1.0f,  1.0f   // 7
        };
        
        const uint32_t indices[] = {
            // передняя грань
            0, 1, 2,  0, 2, 3,
            // задняя грань  
            5, 4, 7,  5, 7, 6,
            // левая грань
            4, 0, 3,  4, 3, 7,
            // правая грань
            1, 5, 6,  1, 6, 2,
            // верхняя грань
            3, 2, 6,  3, 6, 7,
            // нижняя грань
            4, 5, 1,  4, 1, 0
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glGenBuffers(1, &cubeEBO);
        
        glBindVertexArray(cubeVAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glBindVertexArray(0);
        cubeInitialized = true;
    }

    // Размер куба для каждого объекта
    const float cubeSize = 5.0f;
    
    // Цвета для разных типов объектов
    const glm::vec4 colors[] = {
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // Красный
        glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), // Зеленый
        glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), // Синий
        glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // Желтый
        glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), // Пурпурный
        glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), // Голубой
        glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), // Оранжевый
        glm::vec4(0.5f, 0.0f, 1.0f, 1.0f)  // Фиолетовый
    };

    // Рендерим каждый видимый объект как куб
    for (size_t i = 0; i < visibleObjects.size(); i++) {
        const auto& obj = visibleObjects[i];
        
        // Выбираем цвет на основе ID модели
        int colorIndex = obj.modelId % 8;
        if (locCol >= 0) glUniform4fv(locCol, 1, glm::value_ptr(colors[colorIndex]));
        
        // Модельная матрица из позиции и кватерниона
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(obj.x, obj.y, obj.z));
        model = glm::scale(model, glm::vec3(cubeSize));
        
        // Применяем поворот только если кватернион не единичный
        if (m_useQuaternions && (abs(obj.rx) > 0.001f || abs(obj.ry) > 0.001f || abs(obj.rz) > 0.001f || abs(obj.rw - 1.0f) > 0.001f)) {
            // Отладочная информация для первых нескольких объектов
            if (i < 3) {
                //printf("[Cube %zu] Raw: rx=%.3f, ry=%.3f, rz=%.3f, rw=%.3f\n", 
                       //i, obj.rx, obj.ry, obj.rz, obj.rw);
            }
            
            // Используем ту же математику, что и в applyQuaternion
            float rx = obj.rx, ry = obj.ry, rz = obj.rz, rw = obj.rw;
            float length = sqrtf(rx * rx + ry * ry + rz * rz + rw * rw);
            if (length > 0.0001f) {
                rx /= length; ry /= length; rz /= length; rw /= length;
            }
            
            // Создаем матрицу поворота из кватерниона (та же формула, что и в applyQuaternion)
            glm::mat4 rotMatrix = glm::mat4(
                1.0f - 2.0f * (ry * ry + rz * rz),  2.0f * (rx * ry - rw * rz),      2.0f * (rx * rz + rw * ry),      0.0f,
                2.0f * (rx * ry + rw * rz),        1.0f - 2.0f * (rx * rx + rz * rz), 2.0f * (ry * rz - rw * rx),      0.0f,
                2.0f * (rx * rz - rw * ry),        2.0f * (ry * rz + rw * rx),      1.0f - 2.0f * (rx * rx + ry * ry), 0.0f,
                0.0f,                               0.0f,                             0.0f,                             1.0f
            );
            
            model *= rotMatrix;
            if (i < 3)
            {
                //printf("[Cube %zu] Using custom quaternion matrix\n", i);
            }
        } else if (!m_useQuaternions && (abs(obj.rx) > 0.001f || abs(obj.ry) > 0.001f || abs(obj.rz) > 0.001f)) {
            // Углы Эйлера (по умолчанию) - используем как есть
            glm::mat4 rotEuler = glm::rotate(glm::mat4(1.0f), obj.rx, glm::vec3(1,0,0)) *  // X
                                 glm::rotate(glm::mat4(1.0f), obj.ry, glm::vec3(0,1,0)) *  // Y
                                 glm::rotate(glm::mat4(1.0f), obj.rz, glm::vec3(0,0,1)); // Z
            
            model *= rotEuler;
        }

        const glm::mat4 mvp = proj * view * model;
        if (locMVP >= 0) glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // 12 треугольников * 3 вершины = 36
        glBindVertexArray(0);
    }

    //LogRender("RenderGtaObjects: отрендерено " + std::to_string(visibleObjects.size()) + " кубов");
    glUseProgram(0);
}

// Метод для сброса статистики рендеринга
void Renderer::ResetRenderStats() {
    m_totalVertices = 0;
    m_totalPolygons = 0;
    m_dffVertices = 0;
    m_dffPolygons = 0;
    m_skyboxVertices = 0;
    m_skyboxPolygons = 0;
}

// Метод для принудительного обновления видимых объектов
void Renderer::ForceUpdateVisibleObjects() {
    // Обновляем кэш только если действительно нужно
    if (!m_cameraMoved) {
        m_cameraMoved = true;
        // Принудительно обновляем кэш
        GetVisibleDffModels();
        GetVisibleGtaObjects();
    }
}

// Метод для обновления статистики на основе всех объектов (не только видимых)
void Renderer::UpdateRenderStats() {
    // Сбрасываем все счетчики
    m_totalVertices = 0;
    m_totalPolygons = 0;
    m_dffVertices = 0;
    m_dffPolygons = 0;
    m_skyboxVertices = 0;
    m_skyboxPolygons = 0;
    
    // Подсчитываем статистику для DFF моделей отдельно
    for (const auto& instance : m_dffModels) {
        const auto& model = instance.model;
        m_dffVertices += static_cast<int>(model.vertices.size());
        m_dffPolygons += static_cast<int>(model.polygons.size());
    }
    
    // Подсчитываем статистику для GTA объектов
    for (const auto& obj : m_gtaObjects) {
        // Куб: 6 граней (полигонов) и 24 вершины (4 вершины на грань)
        m_totalPolygons += 6;
        m_totalVertices += 24;
    }
    
    // Статистика скайбокса (отдельно)
    if (m_skyboxInitialized) {
        // Скайбокс: 6 граней * 2 треугольника = 12 полигонов, 36 вершин
        m_skyboxPolygons = 12;
        m_skyboxVertices = 36;
    }
    
    // Общая статистика (DFF + GTA + Skybox)
    m_totalVertices = m_dffVertices + m_skyboxVertices;
    m_totalPolygons = m_dffPolygons + m_skyboxPolygons;
    
    // Отладочная информация (закомментировано для производительности)
    //printf("[Renderer] UpdateRenderStats: DFF моделей: %zu, GTA объектов: %zu, вершин: %d, полигонов: %d\n", 
           //m_dffModels.size(), m_gtaObjects.size(), m_totalVertices, m_totalPolygons);
}

// ============================================================================
// DFF MODEL RENDERING IMPLEMENTATION
// ============================================================================

void Renderer::AddDffModel(const dff::DffModel& model, const char* name, float x, float y, float z, float rx, float ry, float rz, float rw) {
    //printf("[Renderer] AddDffModel: получена модель '%s' с %zu вершинами, %zu полигонами, %zu нормалями\n", 
    //       name, model.vertices.size(), model.polygons.size(), model.normals.size());
    
    // Создаем экземпляр модели
    DffModelInstance instance;
    
    // Проверяем валидность модели и выводим предупреждения только для проблемных
    if (model.vertices.empty() || model.polygons.empty()) {
        printf("[Renderer] ⚠️  ПРОБЛЕМНАЯ МОДЕЛЬ '%s':\n", name);
        printf("[Renderer]   - Вершины: %zu, Полигоны: %zu, Нормали: %zu\n", 
               model.vertices.size(), model.polygons.size(), model.normals.size());
        printf("[Renderer]   - Позиция: (%.1f, %.1f, %.1f)\n", x, y, z);
        
        // Если есть вершины, но нет полигонов - пробуем исправить из unpack
        if (!model.vertices.empty() && model.polygons.empty()) {
            printf("[Renderer] 🔧 Попытка исправления модели '%s' из распакованных файлов...\n", name);
            
            dff::DffModel fixedModel = LoadDffFromUnpack(name);
            
            if (!fixedModel.polygons.empty()) {
                printf("[Renderer] ✅ Модель '%s' успешно исправлена: %zu полигонов\n", 
                       name, fixedModel.polygons.size());
                // Используем исправленную модель
                instance.model = fixedModel;
            } else {
                printf("[Renderer] ❌ Не удалось исправить модель '%s' из unpack\n", name);
                // Используем оригинальную модель
                instance.model = model;
            }
        } else {
            // Для полностью пустых моделей используем оригинал
            instance.model = model;
        }
    } else {
        // Для нормальных моделей используем оригинал
        instance.model = model;
    }
    instance.name = name ? name : "unnamed";
    instance.x = x;
    instance.y = y;
    instance.z = z;
    instance.rx = rx;
    instance.ry = ry;
    instance.rz = rz;
    instance.rw = rw;
    
    // Инициализируем OpenGL объекты
    instance.vao = 0;
    instance.vbo = 0;
    instance.normalVBO = 0;
    instance.ebo = 0;
    instance.indexCount = instance.model.polygons.size() * 3; // 3 индекса на треугольник
    instance.normalCount = instance.model.normals.size();
    instance.uploadedToGPU = false;
    
    // Добавляем модель в очередь - загрузим в GPU позже
    m_dffModels.push_back(instance);
    //printf("[Renderer] AddDffModel: модель '%s' добавлена в очередь (всего DFF моделей: %zu)\n", name, m_dffModels.size());
}
// Методы для работы с IMG архивами
void Renderer::SetImgArchives(const std::vector<img::ImgData*>& archives) {
    m_imgArchives = archives;
    //LogImg("SetImgArchives: установлено " + std::to_string(archives.size()) + " IMG архивов для fallback");
    // Выводим список найденных моделей
    //for (const auto& archive : archives) {
    //    printf("[IMG]   - %s\n", archive->getFileName().c_str());
    //}
    
}

void Renderer::ClearImgArchives() {
    m_imgArchives.clear();
    //printf("[Renderer] ClearImgArchives: IMG архивы очищены\n");
}

void Renderer::LoadAllDffModelsToGPU() {
    if (!m_initialized || !m_window) {
        //printf("[Renderer] LoadAllDffModelsToGPU: OpenGL не инициализирован\n");
        return;
    }
    
    //LogModels("LoadAllDffModelsToGPU: начинаем загрузку " + std::to_string(m_dffModels.size()) + " DFF моделей в GPU...");
    
    // Анализируем все модели перед загрузкой
    size_t totalVertices = 0, totalPolygons = 0, totalNormals = 0;
    size_t emptyModels = 0, modelsWithVerticesOnly = 0, modelsWithPolygonsOnly = 0;
    
    for (const auto& instance : m_dffModels) {
        totalVertices += instance.model.vertices.size();
        totalPolygons += instance.model.polygons.size();
        totalNormals += instance.model.normals.size();
        
        if (instance.model.vertices.empty() && instance.model.polygons.empty()) {
            emptyModels++;
        } else if (instance.model.vertices.empty() && !instance.model.polygons.empty()) {
            modelsWithPolygonsOnly++;
        } else if (!instance.model.vertices.empty() && instance.model.polygons.empty()) {
            modelsWithVerticesOnly++;
        }
    }
    

    //LogModels("Общая статистика моделей:");
    //LogModels("  - Всего вершин: " + std::to_string(totalVertices));
    //LogModels("  - Всего полигонов: " + std::to_string(totalPolygons));
    //LogModels("  - Всего нормалей: " + std::to_string(totalNormals));
    //LogModels("  - Полностью пустых моделей: " + std::to_string(emptyModels));
    //LogModels("  - Моделей только с вершинами: " + std::to_string(modelsWithVerticesOnly));
    //LogModels("  - Моделей только с полигонами: " + std::to_string(modelsWithPolygonsOnly));
    //LogModels("  - Моделей с полной геометрией: " + std::to_string(m_dffModels.size() - emptyModels - modelsWithVerticesOnly - modelsWithPolygonsOnly));
    
    // Если есть проблемные модели, показываем их детально
    if (emptyModels > 0 || modelsWithVerticesOnly > 0 || modelsWithPolygonsOnly > 0) {
        //LogWarning("⚠️  ОБНАРУЖЕНЫ ПРОБЛЕМНЫЕ МОДЕЛИ:");
        //LogWarning("  - Полностью пустых: " + std::to_string(emptyModels));
        //LogWarning("  - Только вершины: " + std::to_string(modelsWithVerticesOnly));
        //LogWarning("  - Только полигоны: " + std::to_string(modelsWithPolygonsOnly));
        //LogWarning("  - Всего проблемных: " + std::to_string(emptyModels + modelsWithVerticesOnly + modelsWithPolygonsOnly) + " из " + std::to_string(m_dffModels.size()) + " (" + 
               //std::to_string((float)(emptyModels + modelsWithVerticesOnly + modelsWithPolygonsOnly) / m_dffModels.size() * 100.0f) + "%)");
        
        // Показываем информацию о fallback
        //LogInfo("🔧 СИСТЕМА ИСПРАВЛЕНИЯ АКТИВНА:");
        //LogInfo("  - Проблемные модели будут автоматически исправляться из unpack/");
        //LogInfo("  - Путь: data/unpack/[имя_модели].dff");
    }
    
    //LogModels("Начинаем загрузку в GPU...");
    
    int loadedCount = 0;
    int errorCount = 0;
    
    for (auto& instance : m_dffModels) {
        if (instance.uploadedToGPU) {
            //LogModels("LoadAllDffModelsToGPU: модель '" + instance.name + "' уже загружена в GPU");
            loadedCount++;
            continue;
        }
        
        if (instance.model.vertices.empty() || instance.model.polygons.empty()) {
            //LogWarning("LoadAllDffModelsToGPU: модель '" + instance.name + "' не содержит геометрии");
            //LogWarning("  - Вершины: " + std::to_string(instance.model.vertices.size()) + ", Полигоны: " + std::to_string(instance.model.polygons.size()) + ", Нормали: " + std::to_string(instance.model.normals.size()));
            //LogWarning("  - Позиция: (" + std::to_string(instance.x) + ", " + std::to_string(instance.y) + ", " + std::to_string(instance.z) + ")");
            
            // Анализируем, что именно не так с моделью
            if (instance.model.vertices.empty() && instance.model.polygons.empty()) {
                //LogError("  - ❌ Модель полностью пустая (нет вершин и полигонов)");
            } else if (instance.model.vertices.empty()) {
                //LogError("  - ❌ Есть полигоны (" + std::to_string(instance.model.polygons.size()) + "), но нет вершин");
            } else if (instance.model.polygons.empty()) {
                //LogError("  - ❌ Есть вершины (" + std::to_string(instance.model.vertices.size()) + "), но нет полигонов");
            }
            
            // Проверяем, есть ли нормали
            if (instance.model.normals.empty()) {
                //LogInfo("  - ℹ️  Нормали отсутствуют (будут сгенерированы)");
            } else {
                //LogInfo("  - ℹ️  Есть нормали (" + std::to_string(instance.model.normals.size()) + ")");
            }
            
            errorCount++;
            continue;
        }
        
        //printf("[Renderer] LoadAllDffModelsToGPU: загружаем модель '%s' в GPU...\n", instance.name.c_str());
        
        if (UploadModelToGPU(instance)) {
            loadedCount++;
            //printf("[Renderer] LoadAllDffModelsToGPU: модель '%s' успешно загружена в GPU\n", instance.name.c_str());
        } else {
            errorCount++;
            //LogError("LoadAllDffModelsToGPU: ошибка загрузки модели '" + instance.name + "' в GPU");
        }
    }
    
    //LogModels("LoadAllDffModelsToGPU: завершено. Успешно: " + std::to_string(loadedCount) + ", ошибок: " + std::to_string(errorCount));
}

// Оптимизированная загрузка только видимых DFF моделей в GPU
void Renderer::LoadVisibleDffModelsToGPU() {
    if (!m_initialized || !m_window) {
        return;
    }
    
    // Получаем видимые модели
    const std::vector<DffModelInstance>& visibleModels = GetVisibleDffModels();
    
    int loadedCount = 0;
    int errorCount = 0;
    
    for (const auto& instance : visibleModels) {
        if (instance.uploadedToGPU) {
            continue; // Уже загружена
        }
        
        if (instance.model.vertices.empty() || instance.model.polygons.empty()) {
            errorCount++;
            continue;
        }
        
        // Загружаем модель в GPU
        if (UploadModelToGPU(const_cast<DffModelInstance&>(instance))) {
            loadedCount++;
        } else {
            errorCount++;
        }
    }
    
    //printf("[Renderer] LoadVisibleDffModelsToGPU: загружено %d видимых моделей, ошибок: %d\n", loadedCount, errorCount);
}

void Renderer::RenderDffModels() {
    
    if (m_dffModels.empty()) {
        return;
    }
    
    // Отладочная информация о количестве DFF моделей
    static int renderLogCount = 0;
    renderLogCount++;
    if (renderLogCount % 180 == 0) { // Логируем каждые 180 кадров (примерно раз в 3 секунды)
        LogRender("RenderDffModels: всего DFF моделей: " + std::to_string(m_dffModels.size()));
    }
    

    // Используем шейдер для DFF моделей с освещением
    if (!m_modelShader) return;
    glUseProgram(m_modelShader);

    // Матрицы
    const glm::mat4 proj = BuildPerspective(m_camera.GetFOV(), (float)m_width / (float)m_height, 0.1f, 10000.0f);
    glm::mat4 view(1.0f);
    view = glm::rotate(view, glm::radians(m_camera.GetRotationX()), glm::vec3(1,0,0));
    view = glm::rotate(view, glm::radians(m_camera.GetRotationY()), glm::vec3(0,0,1));
    view = glm::translate(view, glm::vec3(-m_camera.GetX(), -m_camera.GetY(), -m_camera.GetZ()));

    const GLint locMVP = glGetUniformLocation(m_modelShader, "uMVP");
    const GLint locModel = glGetUniformLocation(m_modelShader, "uModel");
    const GLint locPolygonCount = glGetUniformLocation(m_modelShader, "uPolygonCount");
    const GLint locMaxPolygons = glGetUniformLocation(m_modelShader, "uMaxPolygons");
    const GLint locLightPos = glGetUniformLocation(m_modelShader, "uLightPos");
    const GLint locViewPos = glGetUniformLocation(m_modelShader, "uViewPos");
    const GLint locLightAmbient = glGetUniformLocation(m_modelShader, "uLightAmbient");
    const GLint locLightDiffuse = glGetUniformLocation(m_modelShader, "uLightDiffuse");
    const GLint locLightSpecular = glGetUniformLocation(m_modelShader, "uLightSpecular");
    const GLint locMaterialAmbient = glGetUniformLocation(m_modelShader, "uMaterialAmbient");
    const GLint locMaterialDiffuse = glGetUniformLocation(m_modelShader, "uMaterialDiffuse");
    const GLint locMaterialSpecular = glGetUniformLocation(m_modelShader, "uMaterialSpecular");
    const GLint locMaterialShininess = glGetUniformLocation(m_modelShader, "uMaterialShininess");
    
    // Настройки освещения (классические значения)
    if (locLightPos >= 0) glUniform3f(locLightPos, 1000.0f, 1000.0f, 1000.0f);
    if (locViewPos >= 0) glUniform3f(locViewPos, m_camera.GetX(), m_camera.GetY(), m_camera.GetZ());
    if (locLightAmbient >= 0) glUniform3f(locLightAmbient, 0.2f, 0.2f, 0.2f);
    if (locLightDiffuse >= 0) glUniform3f(locLightDiffuse, 0.8f, 0.8f, 0.8f);
    if (locLightSpecular >= 0) glUniform3f(locLightSpecular, 0.5f, 0.5f, 0.5f);
    if (locMaterialAmbient >= 0) glUniform3f(locMaterialAmbient, 0.2f, 0.2f, 0.2f);
    if (locMaterialDiffuse >= 0) glUniform3f(locMaterialDiffuse, 0.8f, 0.8f, 0.8f);
    if (locMaterialSpecular >= 0) glUniform3f(locMaterialSpecular, 0.5f, 0.5f, 0.5f);
    if (locMaterialShininess >= 0) glUniform1f(locMaterialShininess, 32.0f);
    
    // Находим максимальное количество полигонов среди всех моделей для нормализации
    int maxPolygons = 1;
    for (const auto& instance : m_dffModels) {
        maxPolygons = std::max(maxPolygons, static_cast<int>(instance.model.polygons.size()));
    }
    
    // Передаем максимальное количество полигонов в шейдер
    if (locMaxPolygons >= 0) glUniform1i(locMaxPolygons, maxPolygons);
    
    // Убираем статический цвет - теперь цвет будет вычисляться в шейдере на основе количества полигонов

    int renderedCount = 0;
    int totalModels = static_cast<int>(m_dffModels.size());
    int filteredModels = 0;
    
    // Отладочная информация о первых нескольких моделях
    int debugCount = 0;
    for (const auto& instance : m_dffModels) {
        // ПРОВЕРЯЕМ РАДИУС РЕНДЕРИНГА ДЛЯ DFF МОДЕЛЕЙ
        if (!IsObjectInRenderRadius(instance)) {
            filteredModels++;
            continue; // Пропускаем модели вне радиуса
        }
        
        if (debugCount < 3) {
            debugCount++;
        }
        // Загружаем модель в GPU если она не загружена
        if (!instance.uploadedToGPU) {
            if (!UploadModelToGPU(const_cast<DffModelInstance&>(instance))) {
                //LogRender("RenderDffModels: ошибка загрузки модели '" + instance.name + "' в GPU");
                continue;
            }
        }

        if (instance.vao == 0) {
            //LogRender("RenderDffModels: модель '" + instance.name + "' имеет неверный VAO: " + std::to_string(instance.vao));
            continue;
        }

        if (instance.indexCount == 0) {
            //LogRender("RenderDffModels: модель '" + instance.name + "' имеет 0 индексов");
            continue;
        }




        // Модельная матрица из позиции и кватерниона
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(instance.x, instance.y, instance.z));
        
        // Применяем поворот
        if (m_useQuaternions && (abs(instance.rx) > 0.001f || abs(instance.ry) > 0.001f || abs(instance.rz) > 0.001f || abs(instance.rw - 1.0f) > 0.001f)) {
            float rx = instance.rx, ry = instance.ry, rz = instance.rz, rw = instance.rw;
            float length = sqrtf(rx * rx + ry * ry + rz * rz + rw * rw);
            if (length > 0.0001f) {
                rx /= length; ry /= length; rz /= length; rw /= length;
            }
            
            glm::mat4 rotMatrix = glm::mat4(
                1.0f - 2.0f * (ry * ry + rz * rz),  2.0f * (rx * ry - rw * rz),      2.0f * (rx * rz + rw * ry),      0.0f,
                2.0f * (rx * ry + rw * rz),        1.0f - 2.0f * (rx * rx + rz * rz), 2.0f * (ry * rz - rw * rx),      0.0f,
                2.0f * (rx * rz - rw * ry),        2.0f * (ry * rz + rw * rx),      1.0f - 2.0f * (rx * rx + ry * ry), 0.0f,
                0.0f,                               0.0f,                             0.0f,                             1.0f
            );
            
            model *= rotMatrix;
        }

        const glm::mat4 mvp = proj * view * model;
        if (locMVP >= 0) glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        
        // Передаем количество полигонов для этой конкретной модели
        if (locPolygonCount >= 0) glUniform1i(locPolygonCount, static_cast<int>(instance.model.polygons.size()));

        // Рендерим реальную DFF модель используя её VAO/VBO
        glBindVertexArray(instance.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(instance.indexCount), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        renderedCount++;
    }
    
    // Логируем статистику рендеринга DFF моделей
    static int logFrameCount = 0;
    logFrameCount++;
    if (logFrameCount % 60 == 0) { // Логируем каждые 60 кадров (примерно раз в секунду)
        LogRender("DFF рендеринг: всего " + std::to_string(totalModels) + 
                 ", отфильтровано по радиусу " + std::to_string(filteredModels) + 
                 ", отрендерено " + std::to_string(renderedCount) + 
                 " (радиус: " + std::to_string(m_renderRadius) + ")");
    }
    
    glUseProgram(0);
}

// Методы фильтрации объектов по радиусу рендеринга
bool Renderer::IsObjectInRenderRadius(const DffModelInstance& obj) const {
    // Получаем позицию камеры
    float camX = m_camera.GetX();
    float camY = m_camera.GetY();
    
    // Вычисляем расстояние по X и Y (игнорируем Z)
    float dx = obj.x - camX;
    float dy = obj.y - camY;
    float distanceSquared = dx * dx + dy * dy;
    float distance = sqrt(distanceSquared);
    
    // Отладочная информация для первых нескольких DFF моделей
    static int debugCount = 0;
    if (debugCount < 5) {
        LogRender("IsObjectInRenderRadius DFF: модель '" + obj.name + "' на расстоянии " + std::to_string(distance) + " от камеры (" + std::to_string(camX) + ", " + std::to_string(camY) + ")");
        LogRender("  Позиция модели: (" + std::to_string(obj.x) + ", " + std::to_string(obj.y) + ")");
        LogRender("  Рендер-радиус: " + std::to_string(m_renderRadius));
        LogRender("  В радиусе: " + std::string(distance <= m_renderRadius ? "ДА" : "НЕТ"));
        debugCount++;
    }
    
    // Проверяем, находится ли объект в радиусе
    return distanceSquared <= m_renderRadius * m_renderRadius;
}

bool Renderer::IsObjectInRenderRadius(const ipl::IplObject& obj) const {
    // Получаем позицию камеры
    float camX = m_camera.GetX();
    float camY = m_camera.GetY();
    
    // Вычисляем расстояние по X и Y (игнорируем Z)
    float dx = obj.x - camX;
    float dy = obj.y - camY;
    float distanceSquared = dx * dx + dy * dy;
    float distance = sqrt(distanceSquared);
    
    // Отладочная информация для первых нескольких объектов
    static int debugCount = 0;
    if (debugCount < 5) {
        //LogRender("IsObjectInRenderRadius: объект '" + obj.name + "' на расстоянии " + std::to_string(distance) + " от камеры (" + std::to_string(camX) + ", " + std::to_string(camY) + ")");
        //LogRender("  Позиция объекта: (" + std::to_string(obj.x) + ", " + std::to_string(obj.y) + ")");
        //LogRender("  Рендер-радиус: " + std::to_string(m_renderRadius));
        //LogRender("  В радиусе: " + std::string(distance <= m_renderRadius ? "ДА" : "НЕТ"));
        debugCount++;
    }
    
    // Проверяем, находится ли объект в радиусе
    return distanceSquared <= m_renderRadius * m_renderRadius;
}

const std::vector<Renderer::DffModelInstance>& Renderer::GetVisibleDffModels() const {
    // Проверяем, сдвинулась ли камера или принудительно обновляем
    float currentCamX = m_camera.GetX();
    float currentCamY = m_camera.GetY();
    
    // Обновляем кэш если камера сдвинулась или принудительно обновляем
    if (m_cameraMoved || abs(currentCamX - m_lastCameraX) > 5.0f || abs(currentCamY - m_lastCameraY) > 5.0f) {
        m_lastCameraX = currentCamX;
        m_lastCameraY = currentCamY;
        m_cameraMoved = false; // Сбрасываем флаг
        
        // Очищаем и пересчитываем кэш
        m_visibleDffModels.clear();

        for (const auto& model : m_dffModels) {
            if (IsObjectInRenderRadius(model)) {
                m_visibleDffModels.push_back(model);
                //LogRender("GetVisibleDffModels: модель '" + model.name + "' В РАДИУСЕ (" + std::to_string(model.x) + ", " + std::to_string(model.y) + ")");
            } else {
                float distance = sqrt((model.x - currentCamX) * (model.x - currentCamX) + (model.y - currentCamY) * (model.y - currentCamY));
                //LogRender("GetVisibleDffModels: модель '" + model.name + "' ВНЕ РАДИУСА (" + std::to_string(model.x) + ", " + std::to_string(model.y) + ") - расстояние: " + std::to_string(distance) + " > " + std::to_string(m_renderRadius));
            }
        }
        
        // Логируем статистику фильтрации
        static int filterLogCount = 0;
        filterLogCount++;
        if (filterLogCount % 120 == 0) { // Логируем каждые 120 кадров (примерно раз в 2 секунды)
            LogRender("Фильтрация DFF моделей: всего " + std::to_string(m_dffModels.size()) + 
                     ", видимых " + std::to_string(m_visibleDffModels.size()) + 
                     ", отфильтровано " + std::to_string(m_dffModels.size() - m_visibleDffModels.size()) + 
                     " (радиус: " + std::to_string(m_renderRadius) + ")");
        }

    }
    
    return m_visibleDffModels;
}

const std::vector<ipl::IplObject>& Renderer::GetVisibleGtaObjects() const {
    // Проверяем, сдвинулась ли камера или принудительно обновляем
    float currentCamX = m_camera.GetX();
    float currentCamY = m_camera.GetY();
    
    // Обновляем кэш если камера сдвинулась или принудительно обновляем
    if (m_cameraMoved || abs(currentCamX - m_lastCameraX) > 5.0f || abs(currentCamY - m_lastCameraY) > 5.0f) {
        m_lastCameraX = currentCamX;
        m_lastCameraY = currentCamY;
        m_cameraMoved = false; // Сбрасываем флаг
        
        // Очищаем и пересчитываем кэш
        m_visibleGtaObjects.clear();
        

        
        for (const auto& obj : m_gtaObjects) {
            if (IsObjectInRenderRadius(obj)) {
                m_visibleGtaObjects.push_back(obj);
               // //LogRender("GetVisibleGtaObjects: объект '" + obj.name + "' В РАДИУСЕ (" + std::to_string(obj.x) + ", " + std::to_string(obj.y) + ")");
            } else {
               // float distance = sqrt((obj.x - currentCamX) * (obj.x - currentCamX) + (obj.y - currentCamY) * (obj.y - currentCamY));
              //  //LogRender("GetVisibleGtaObjects: объект '" + obj.name + "' ВНЕ РАДИУСА (" + std::to_string(obj.x) + ", " + std::to_string(obj.y) + ") - расстояние: " + std::to_string(distance) + " > " + std::to_string(m_renderRadius));
            }
        }
        

    }
    
    return m_visibleGtaObjects;
}

// ============================================================================
// MODERN OPENGL IMPLEMENTATION (VBO/VAO)
// ============================================================================

bool Renderer::UploadModelToGPU(DffModelInstance& instance) {
    // Создаем не-const копию модели для вызова loadToGPU
    dff::DffModel& model = const_cast<dff::DffModel&>(instance.model);
    
    //printf("[Renderer] UploadModelToGPU: начинаем загрузку модели '%s' в GPU\n", instance.name.c_str());
    
    if (model.vertices.empty() || model.polygons.empty()) {
        //LogWarning("UploadModelToGPU: модель '" + instance.name + "' не содержит геометрии - ПРОПУСКАЕМ");
        //LogWarning("  - Вершины: " + std::to_string(model.vertices.size()) + ", Полигоны: " + std::to_string(model.polygons.size()) + ", Нормали: " + std::to_string(model.normals.size()));
        return false;
    }
    
    //LogRender("UploadModelToGPU: модель '" + instance.name + "' содержит " + std::to_string(model.vertices.size()) + " вершин и " + std::to_string(model.polygons.size()) + " полигонов");
    
    //printf("[Renderer] UploadModelToGPU: модель '%s' содержит %zu вершин и %zu полигонов\n", instance.name.c_str(), model.vertices.size(), model.polygons.size());
    
    // Без текущего контекста OpenGL буферы создавать нельзя
    if (!m_initialized || !m_window || glfwGetCurrentContext() == nullptr) {
        //LogError("UploadModelToGPU: OpenGL не готов (m_initialized=" + std::to_string(m_initialized) + ", m_window=" + std::to_string((uintptr_t)m_window) + ", context=" + std::to_string((uintptr_t)glfwGetCurrentContext()) + ")");
        return false;
    }
    
    //printf("[Renderer] UploadModelToGPU: OpenGL готов, используем метод loadToGPU() из DffModel...\n");
    
    // Используем наш метод loadToGPU() из DffModel
    if (model.loadToGPU()) {
        //LogRender("UploadModelToGPU: модель '" + instance.name + "' успешно загружена в GPU через DffModel::loadToGPU()");
        
        // Копируем OpenGL объекты из DffModel в instance
        instance.vao = model.vao;
        instance.vbo = model.vbo;
        instance.ebo = model.ebo;
        instance.uploadedToGPU = true;
        
        //LogRender("UploadModelToGPU: скопированы OpenGL объекты - VAO=" + std::to_string(instance.vao) + ", VBO=" + std::to_string(instance.vbo) + ", EBO=" + std::to_string(instance.ebo));
        
        return true;
    } else {
        //LogError("UploadModelToGPU: ОШИБКА - DffModel::loadToGPU() не удался для модели '" + instance.name + "'");
        return false;
    }
}

void Renderer::DeleteModelFromGPU(DffModelInstance& instance) {
    //LogRender("DeleteModelFromGPU: удаляем модель '" + instance.name + "' из GPU");
    //LogRender("  - Удаляем VAO: " + std::to_string(instance.vao) + ", VBO: " + std::to_string(instance.vbo) + ", NormalVBO: " + std::to_string(instance.normalVBO) + ", EBO: " + std::to_string(instance.ebo));
    
    if (instance.vao != 0) {
        glDeleteVertexArrays(1, &instance.vao);
        instance.vao = 0;
    }
    
    if (instance.vbo != 0) {
        glDeleteBuffers(1, &instance.vbo);
        instance.vbo = 0;
    }
    
    if (instance.normalVBO != 0) {
        glDeleteBuffers(1, &instance.normalVBO);
        instance.normalVBO = 0;
    }
    
    if (instance.ebo != 0) {
        glDeleteBuffers(1, &instance.ebo);
        instance.ebo = 0;
    }
    
    instance.uploadedToGPU = false;
    instance.indexCount = 0;
    //LogRender("DeleteModelFromGPU: модель '" + instance.name + "' удалена из GPU");
}

void Renderer::CleanupAllGPUModels() {
    //printf("[Renderer] Очистка всех GPU моделей...\n");
    
    for (auto& instance : m_dffModels) {
        if (instance.uploadedToGPU) {
            DeleteModelFromGPU(instance);
        }
    }
    
    //printf("[Renderer] Все GPU модели очищены\n");
}

// Fallback метод для загрузки проблемных моделей из распакованных файлов
dff::DffModel Renderer::LoadDffFromUnpack(const std::string& modelName) {
    // Формируем путь к распакованному файлу
    std::string unpackPath = "unpack\\" + modelName + ".dff";
    
    // Проверяем существование файла
    FILE* file = fopen(unpackPath.c_str(), "rb");
    if (!file) {
        // Пытаемся извлечь модель из IMG архива
        if (ExtractModelFromImg(modelName, unpackPath)) {
            // Теперь пытаемся открыть извлеченный файл
            file = fopen(unpackPath.c_str(), "rb");
            if (!file) {
                return dff::DffModel(); // Возвращаем пустую модель
            }
        } else {
            return dff::DffModel(); // Возвращаем пустую модель
        }
    }
    
    // Получаем размер файла
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (fileSize <= 0) {
        fclose(file);
        return dff::DffModel();
    }
    
    // Читаем файл в буфер
    std::vector<uint8_t> fileData(fileSize);
    size_t bytesRead = fread(fileData.data(), 1, fileSize, file);
    fclose(file);
    
    if (bytesRead != static_cast<size_t>(fileSize)) {
        return dff::DffModel();
    }
    
    // Пытаемся загрузить DFF из буфера
    dff::DffData dffData;
    if (dff::loadDffFromBuffer(fileData, dffData, modelName.c_str())) {
        const dff::DffModel& model = dffData.getModel();
        return model;
    } else {
        return dff::DffModel();
    }
}

// Функция для извлечения модели из IMG архива в файл
bool Renderer::ExtractModelFromImg(const std::string& modelName, const std::string& outputPath) {
    // Создаем папку unpack, если её нет
    system("mkdir unpack 2>nul");
    
    if (m_imgArchives.empty()) {
        return false;
    }
    
    try {
        // Добавляем расширение .dff, если его нет
        std::string dffFileName = modelName;
        if (dffFileName.find(".dff") == std::string::npos) {
            dffFileName += ".dff";
        }
        
        // Ищем модель в IMG архивах
        if (!img::modelExists(m_imgArchives, dffFileName)) {
            return false;
        }
        
        // Получаем данные модели из IMG
        std::vector<uint8_t> modelData = img::getModelData(m_imgArchives, dffFileName);
        
        if (modelData.empty()) {
            return false;
        }
        
        // Сохраняем данные в файл
        FILE* outputFile = fopen(outputPath.c_str(), "wb");
        if (!outputFile) {
            return false;
        }
        
        size_t written = fwrite(modelData.data(), 1, modelData.size(), outputFile);
        fclose(outputFile);
        
        if (written != modelData.size()) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

// ============================================================================
// SKYBOX IMPLEMENTATION
// ============================================================================

bool Renderer::CreateSkyboxResources() {
    // Вершины куба для скайбокса (большой размер, чтобы покрыть всю сцену)
    const float size = 5000.0f;
    
    // Создаем все 36 вершин для 6 граней куба (каждая грань = 2 треугольника = 6 вершин)
    const float vertices[] = {
        // Задняя грань (Z = -size)
        -size, -size, -size,  // 0
         size, -size, -size,  // 1
         size,  size, -size,  // 2
         size,  size, -size,  // 3
        -size,  size, -size,  // 4
        -size, -size, -size,  // 5
        
        // Передняя грань (Z = +size)
        -size, -size,  size,  // 6
         size, -size,  size,  // 7
         size,  size,  size,  // 8
         size,  size,  size,  // 9
        -size,  size,  size,  // 10
        -size, -size,  size,  // 11
        
        // Левая грань (X = -size)
        -size, -size, -size,  // 12
        -size, -size,  size,  // 13
        -size,  size,  size,  // 14
        -size,  size,  size,  // 15
        -size,  size, -size,  // 16
        -size, -size, -size,  // 17
        
        // Правая грань (X = +size)
         size, -size, -size,  // 18
         size, -size,  size,  // 19
         size,  size,  size,  // 20
         size,  size,  size,  // 21
         size,  size, -size,  // 22
         size, -size, -size,  // 23
        
        // Нижняя грань (Y = -size)
        -size, -size, -size,  // 24
         size, -size, -size,  // 25
         size, -size,  size,  // 26
         size, -size,  size,  // 27
        -size, -size,  size,  // 28
        -size, -size, -size,  // 29
        
        // Верхняя грань (Y = +size)
        -size,  size, -size,  // 30
         size,  size, -size,  // 31
         size,  size,  size,  // 32
         size,  size,  size,  // 33
        -size,  size,  size,  // 34
        -size,  size, -size   // 35
    };

    // Создаем VAO
    glGenVertexArrays(1, &m_skyboxVAO);
    glGenBuffers(1, &m_skyboxVBO);

    glBindVertexArray(m_skyboxVAO);
    
    // Загружаем вершины
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Настраиваем атрибуты
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    // Загружаем текстуры скайбокса
    if (!LoadSkyboxTextures()) {
        //LogError("ОШИБКА: Не удалось загрузить текстуры скайбокса");
        return false;
    }

    // Создаем шейдер для скайбокса с текстурами
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 uProjection;
        uniform mat4 uView;
        
        out vec3 TexCoords;
        
        void main() {
            TexCoords = aPos;
            vec4 pos = uProjection * uView * vec4(aPos, 1.0);
            gl_Position = pos.xyww; // z = w для скайбокса
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 TexCoords;
        
        uniform samplerCube uSkybox;
        
        void main() {
            // Используем текстуру скайбокса
            FragColor = texture(uSkybox, TexCoords);
        }
    )";

    // Компилируем шейдеры
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    if (!vertexShader || !fragmentShader) {
        return false;
    }

    // Линкуем программу
    m_skyboxShader = LinkProgram(vertexShader, fragmentShader);
    
    if (!m_skyboxShader) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Очищаем шейдеры
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    m_skyboxInitialized = true;
    return true;
}

void Renderer::DestroySkyboxResources() {
    if (m_skyboxVAO) {
        glDeleteVertexArrays(1, &m_skyboxVAO);
        m_skyboxVAO = 0;
    }
    
    if (m_skyboxVBO) {
        glDeleteBuffers(1, &m_skyboxVBO);
        m_skyboxVBO = 0;
    }
    
    if (m_skyboxShader) {
        glDeleteProgram(m_skyboxShader);
        m_skyboxShader = 0;
    }
    
    if (m_skyboxTexture) {
        glDeleteTextures(1, &m_skyboxTexture);
        m_skyboxTexture = 0;
    }
    
    m_skyboxInitialized = false;
}

void Renderer::RenderSkybox() {
    if (!m_skyboxInitialized || !m_skyboxShader || !m_skyboxVAO) {
        return;
    }

    // Отключаем тест глубины для скайбокса
    glDisable(GL_DEPTH_TEST);
    
    // Используем шейдер скайбокса
    glUseProgram(m_skyboxShader);
    
    // Привязываем текстуру скайбокса
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
    
    // Передаем текстуру в шейдер
    GLint skyboxLocation = glGetUniformLocation(m_skyboxShader, "uSkybox");
    if (skyboxLocation >= 0) {
        glUniform1i(skyboxLocation, 0);
    }
    
    // Матрицы для скайбокса (без переноса камеры)
    const glm::mat4 proj = BuildPerspective(m_camera.GetFOV(), (float)m_width / (float)m_height, 0.1f, 10000.0f);
    glm::mat4 view = glm::mat4(1.0f);
    
    // Только вращение камеры, без переноса
    view = glm::rotate(view, glm::radians(m_camera.GetRotationX()), glm::vec3(1,0,0));
    view = glm::rotate(view, glm::radians(m_camera.GetRotationY()), glm::vec3(0,0,1));
    
    // Передаем матрицы в шейдер
    GLint projLocation = glGetUniformLocation(m_skyboxShader, "uProjection");
    GLint viewLocation = glGetUniformLocation(m_skyboxShader, "uView");
    
    if (projLocation >= 0) glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(proj));
    if (viewLocation >= 0) glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
    
    // Рендерим скайбокс
    glBindVertexArray(m_skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36); // 6 граней * 2 треугольника * 3 вершины = 36 вершин
    glBindVertexArray(0);
    
    // Восстанавливаем тест глубины
    glEnable(GL_DEPTH_TEST);
    
    glUseProgram(0);
}

// ============================================================================
// SKYBOX TEXTURE LOADING
// ============================================================================

bool Renderer::LoadSkyboxTextures() {
    // Создаем кубическую текстуру
    glGenTextures(1, &m_skyboxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
    
    // Настройки фильтрации для кубической текстуры
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    

    
    // Загружаем каждую грань
    for (int i = 0; i < 6; ++i) {

        const int size = 1;
        std::vector<unsigned char> textureData(size * size * 3);
        
        // Создаем простую текстуру в зависимости от грани
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int index = (y * size + x) * 3;
                
                // Разные цвета для разных граней для отладки
                switch (i) {
                    case 0: // right - красный
                        textureData[index] = 255;     // R
                        textureData[index + 1] = 192;   // G
                        textureData[index + 2] = 154;   // B
                        break;
                    case 1: // left - зеленый
                        textureData[index] = 255;     // R
                        textureData[index + 1] = 192;   // G
                        textureData[index + 2] = 154;   // B
                        break;
                    case 2: // top - синий
                        textureData[index] = 255;     // R
                        textureData[index + 1] = 192;   // G
                        textureData[index + 2] = 154;   // B
                        break;
                    case 3: // bottom - желтый
                        textureData[index] = 255;     // R
                        textureData[index + 1] = 192;   // G
                        textureData[index + 2] = 154;   // B
                        break;
                    case 4: // front - пурпурный
                        textureData[index] = 255;     // R
                        textureData[index + 1] = 192;   // G
                        textureData[index + 2] = 154;   // B
                        break;
                    case 5: // back - голубой
                        textureData[index] = 255;     // R
                        textureData[index + 1] = 192;   // G
                        textureData[index + 2] = 154;   // B
                        break;
                }
            }
        }
        
        // Загружаем текстуру в OpenGL
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
    }
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    //LogRender("Скайбокс текстуры загружены успешно");
    return true;
}

// ============================================================================
// GEOMETRY DUMPING
// ============================================================================

bool Renderer::DumpGeometryToFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LogRender("Ошибка: не удалось открыть файл " + filename + " для записи");
        return false;
    }
    
    // Записываем сигнатуру файла: "gemometry_ms"
    const char* fileSignature = "gemometry_ms";
    file.write(fileSignature, 12);
    
    // Записываем количество моделей (uint32_t, little-endian)
    uint32_t modelCount = static_cast<uint32_t>(m_dffModels.size());
    file.write(reinterpret_cast<const char*>(&modelCount), sizeof(uint32_t));
    
    LogRender("Начинаем дамп " + std::to_string(modelCount) + " моделей в файл " + filename);
    
    // Дамп каждой модели
    for (const auto& instance : m_dffModels) {
        // Записываем длину названия модели (uint32_t)
        uint32_t nameLength = static_cast<uint32_t>(instance.name.length());
        file.write(reinterpret_cast<const char*>(&nameLength), sizeof(uint32_t));
        
        // Записываем название модели
        file.write(instance.name.c_str(), nameLength);
        
        // Записываем координаты модели (7 float, little-endian)
        file.write(reinterpret_cast<const char*>(&instance.x), sizeof(float));
        file.write(reinterpret_cast<const char*>(&instance.y), sizeof(float));
        file.write(reinterpret_cast<const char*>(&instance.z), sizeof(float));
        file.write(reinterpret_cast<const char*>(&instance.rx), sizeof(float));
        file.write(reinterpret_cast<const char*>(&instance.ry), sizeof(float));
        file.write(reinterpret_cast<const char*>(&instance.rz), sizeof(float));
        file.write(reinterpret_cast<const char*>(&instance.rw), sizeof(float));
        
        // Записываем количество треугольников (uint32_t)
        const auto& model = instance.model;
        uint32_t triangleCount = static_cast<uint32_t>(model.polygons.size());
        file.write(reinterpret_cast<const char*>(&triangleCount), sizeof(uint32_t));
        
        // Записываем треугольники (каждый треугольник = 9 float, little-endian)
        for (const auto& polygon : model.polygons) {
            const auto& v1 = model.vertices[polygon.vertex1];
            const auto& v2 = model.vertices[polygon.vertex2];
            const auto& v3 = model.vertices[polygon.vertex3];
            
            // Вершина 1
            file.write(reinterpret_cast<const char*>(&v1.x), sizeof(float));
            file.write(reinterpret_cast<const char*>(&v1.y), sizeof(float));
            file.write(reinterpret_cast<const char*>(&v1.z), sizeof(float));
            
            // Вершина 2
            file.write(reinterpret_cast<const char*>(&v2.x), sizeof(float));
            file.write(reinterpret_cast<const char*>(&v2.y), sizeof(float));
            file.write(reinterpret_cast<const char*>(&v2.z), sizeof(float));
            
            // Вершина 3
            file.write(reinterpret_cast<const char*>(&v3.x), sizeof(float));
            file.write(reinterpret_cast<const char*>(&v3.y), sizeof(float));
            file.write(reinterpret_cast<const char*>(&v3.z), sizeof(float));
        }
    }
    
    file.close();
    
    LogRender("Геометрия успешно дамплена в файл " + filename + 
              " (моделей: " + std::to_string(modelCount) + 
              ", общий размер: " + std::to_string(file.tellp()) + " байт)");
    
    return true;
}

