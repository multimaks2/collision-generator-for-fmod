#pragma once

// GLEW ДОЛЖЕН быть первым OpenGL заголовком!
#include "../vendor/glew-2.2.0/include/GL/glew.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

// Forward declaration
class Renderer;

class Input {
public:
    Input();
    ~Input();
    
    // Инициализация
    void Initialize(GLFWwindow* window, Renderer* renderer);
    void Shutdown();
    
    // Обновление состояния
    void Update();
    
    // Состояние мыши
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonJustPressed(int button) const;
    bool IsMouseButtonJustReleased(int button) const;
    glm::vec2 GetMousePosition() const;
    glm::vec2 GetMouseDelta() const;
    float GetMouseScroll() const;
    
    // Состояние клавиатуры
    bool IsKeyPressed(int key) const;
    bool IsKeyJustPressed(int key) const;
    bool IsKeyJustReleased(int key) const;
    
    // Проверка ImGui
    bool IsImGuiHovered() const;
    
private:
    // Callback функции (приватные)
    static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void OnMouseMove(GLFWwindow* window, double xpos, double ypos);
    static void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset);
    static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void OnFramebufferSize(GLFWwindow* window, int width, int height);
    static void OnError(int error, const char* description);
    
    // Внутренние методы
    void UpdateMouseState();
    void UpdateKeyboardState();
    
private:
    // Ссылки
    GLFWwindow* m_window;
    Renderer* m_renderer;
    
    // Состояние мыши
    glm::vec2 m_mousePosition;
    glm::vec2 m_mouseDelta;
    glm::vec2 m_lastMousePosition;
    float m_mouseScroll;
    
    // Состояние кнопок мыши
    std::map<int, bool> m_mouseButtonStates;
    std::map<int, bool> m_mouseButtonJustPressed;
    std::map<int, bool> m_mouseButtonJustReleased;
    
    // Состояние клавиатуры
    std::map<int, bool> m_keyStates;
    std::map<int, bool> m_keyJustPressed;
    std::map<int, bool> m_keyJustReleased;
    
    // Флаги состояния
    bool m_initialized;
    bool m_leftMousePressed;
    
    // Статические экземпляры для callback'ов
    static Input* s_instance;
};
