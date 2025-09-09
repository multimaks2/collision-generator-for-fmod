#include "Input.h"
#include "Renderer.h"
#include <iostream>

// Статический экземпляр для callback'ов
Input* Input::s_instance = nullptr;

Input::Input() : m_window(nullptr), m_renderer(nullptr), m_initialized(false), m_leftMousePressed(false),
    m_mousePosition(0.0f), m_mouseDelta(0.0f), m_lastMousePosition(0.0f), m_mouseScroll(0.0f) {
}

Input::~Input() {
    Shutdown();
}

void Input::Initialize(GLFWwindow* window, Renderer* renderer) {
    if (m_initialized) return;
    
    m_window = window;
    m_renderer = renderer;
    
    // Устанавливаем статический экземпляр
    s_instance = this;
    
    // Устанавливаем callback функции
    glfwSetMouseButtonCallback(m_window, OnMouseButton);
    glfwSetCursorPosCallback(m_window, OnMouseMove);
    glfwSetScrollCallback(m_window, OnMouseScroll);
    glfwSetKeyCallback(m_window, OnKey);
    glfwSetFramebufferSizeCallback(m_window, OnFramebufferSize);
    glfwSetErrorCallback(OnError);
    
    // Устанавливаем user pointer для доступа к рендереру
    glfwSetWindowUserPointer(m_window, renderer);
    
    m_initialized = true;
    std::cout << "[Input] Система ввода успешно инициализирована" << std::endl;
}

void Input::Shutdown() {
    if (!m_initialized) return;
    
    // Сбрасываем callback функции
    if (m_window) {
        glfwSetMouseButtonCallback(m_window, nullptr);
        glfwSetCursorPosCallback(m_window, nullptr);
        glfwSetScrollCallback(m_window, nullptr);
        glfwSetKeyCallback(m_window, nullptr);
        glfwSetFramebufferSizeCallback(m_window, nullptr);
        glfwSetErrorCallback(nullptr);
    }
    
    s_instance = nullptr;
    m_initialized = false;
    std::cout << "[Input] Система ввода завершена" << std::endl;
}

void Input::Update() {
    if (!m_initialized) return;
    
    // Обновляем состояние мыши
    UpdateMouseState();
    
    // Обновляем состояние клавиатуры
    UpdateKeyboardState();
}

// ====================================================================================================
// CALLBACK ФУНКЦИИ (статические)
// ====================================================================================================

void Input::OnMouseButton(GLFWwindow* window, int button, int action, int mods) {
    if (!s_instance) return;
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Проверяем, не находится ли курсор над ImGui элементом
            if (s_instance->m_renderer && s_instance->m_renderer->IsImGuiHovered()) {
                // Если курсор над ImGui, не активируем управление камерой
                return;
            }
            
            s_instance->m_leftMousePressed = true;
            s_instance->m_mouseButtonStates[button] = true;
            s_instance->m_mouseButtonJustPressed[button] = true;
            
            // Скрываем курсор при нажатии ЛКМ
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (action == GLFW_RELEASE) {
            s_instance->m_leftMousePressed = false;
            s_instance->m_mouseButtonStates[button] = false;
            s_instance->m_mouseButtonJustReleased[button] = true;
            
            // Показываем курсор при отпускании ЛКМ
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    } else {
        // Обрабатываем другие кнопки мыши
        if (action == GLFW_PRESS) {
            s_instance->m_mouseButtonStates[button] = true;
            s_instance->m_mouseButtonJustPressed[button] = true;
        } else if (action == GLFW_RELEASE) {
            s_instance->m_mouseButtonStates[button] = false;
            s_instance->m_mouseButtonJustReleased[button] = true;
        }
    }
}

void Input::OnMouseMove(GLFWwindow* window, double xpos, double ypos) {
    if (!s_instance) return;
    
    s_instance->m_mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    
    if (s_instance->m_leftMousePressed) {
        // Проверяем, не находится ли курсор над ImGui элементом
        if (s_instance->m_renderer && s_instance->m_renderer->IsImGuiHovered()) {
            // Если курсор над ImGui, не обновляем камеру
            return;
        }
        
        // Вычисляем дельту движения мыши
        s_instance->m_mouseDelta = s_instance->m_mousePosition - s_instance->m_lastMousePosition;
        
        // Обновляем камеру через рендерер
        if (s_instance->m_renderer) {
            s_instance->m_renderer->UpdateCameraFromMouse(s_instance->m_mouseDelta.x, s_instance->m_mouseDelta.y);
        }
    }
    
    s_instance->m_lastMousePosition = s_instance->m_mousePosition;
}

void Input::OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
    if (!s_instance) return;
    
    // Проверяем, не находится ли курсор над ImGui элементом
    if (s_instance->m_renderer && s_instance->m_renderer->IsImGuiHovered()) {
        // Если курсор над ImGui, не изменяем FOV
        return;
    }
    
    s_instance->m_mouseScroll = static_cast<float>(yoffset);
    
    // Обновляем FOV через рендерер
    if (s_instance->m_renderer) {
        s_instance->m_renderer->UpdateFOVFromScroll(yoffset);
    }
}

void Input::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!s_instance) return;
    
    if (action == GLFW_PRESS) {
        s_instance->m_keyStates[key] = true;
        s_instance->m_keyJustPressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        s_instance->m_keyStates[key] = false;
        s_instance->m_keyJustReleased[key] = true;
    }
}

void Input::OnFramebufferSize(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Input::OnError(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// ====================================================================================================
// ВНУТРЕННИЕ МЕТОДЫ
// ====================================================================================================

void Input::UpdateMouseState() {
    // Сбрасываем флаги "только что нажато/отпущено"
    for (auto& [button, state] : m_mouseButtonJustPressed) {
        state = false;
    }
    for (auto& [button, state] : m_mouseButtonJustReleased) {
        state = false;
    }
    
    // Сбрасываем дельту мыши
    m_mouseDelta = glm::vec2(0.0f);
    
    // Сбрасываем прокрутку мыши
    m_mouseScroll = 0.0f;
}

void Input::UpdateKeyboardState() {
    // Сбрасываем флаги "только что нажато/отпущено"
    for (auto& [key, state] : m_keyJustPressed) {
        state = false;
    }
    for (auto& [key, state] : m_keyJustReleased) {
        state = false;
    }
}

// ====================================================================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ====================================================================================================

bool Input::IsMouseButtonPressed(int button) const {
    auto it = m_mouseButtonStates.find(button);
    return it != m_mouseButtonStates.end() ? it->second : false;
}

bool Input::IsMouseButtonJustPressed(int button) const {
    auto it = m_mouseButtonJustPressed.find(button);
    return it != m_mouseButtonJustPressed.end() ? it->second : false;
}

bool Input::IsMouseButtonJustReleased(int button) const {
    auto it = m_mouseButtonJustReleased.find(button);
    return it != m_mouseButtonJustReleased.end() ? it->second : false;
}

glm::vec2 Input::GetMousePosition() const {
    return m_mousePosition;
}

glm::vec2 Input::GetMouseDelta() const {
    return m_mouseDelta;
}

float Input::GetMouseScroll() const {
    return m_mouseScroll;
}

bool Input::IsKeyPressed(int key) const {
    auto it = m_keyStates.find(key);
    return it != m_keyStates.end() ? it->second : false;
}

bool Input::IsKeyJustPressed(int key) const {
    auto it = m_keyJustPressed.find(key);
    return it != m_keyJustPressed.end() ? it->second : false;
}

bool Input::IsKeyJustReleased(int key) const {
    auto it = m_keyJustReleased.find(key);
    return it != m_keyJustReleased.end() ? it->second : false;
}



bool Input::IsImGuiHovered() const {
    if (m_renderer) {
        return m_renderer->IsImGuiHovered();
    }
    return false;
}
