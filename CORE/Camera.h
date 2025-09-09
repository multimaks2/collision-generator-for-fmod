#pragma once

#include <GLFW/glfw3.h>
#include <cmath>

class Camera {
public:
    Camera();
    ~Camera();
    
    // Инициализация и настройка
    void Initialize(float x, float y, float z, float rotX, float rotY, float fov);
    void SetPosition(float x, float y, float z);
    void SetRotation(float rotX, float rotY);
    void SetFOV(float fov);
    
    // Движение камеры
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);
    void Rotate(float deltaX, float deltaY);
    void UpdateFOV(float delta);
    void UpdateFOVInertia();
    
    // Плавное движение камеры
    void MoveForwardSmooth(float distance);
    void MoveRightSmooth(float distance);
    void MoveUpSmooth(float distance);
    void RotateSmooth(float deltaX, float deltaY);
    void UpdateMovementInertia();
    
    // Получение информации о камере (для ImGui)
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetZ() const { return m_z; }
    float GetRotationX() const { return m_rotX; }
    float GetRotationY() const { return m_rotY; }
    float GetFOV() const { return m_fov; }
    
    // Получение векторов направления
    void GetForwardVector(float& x, float& y, float& z) const;
    void GetRightVector(float& x, float& y, float& z) const;
    void GetUpVector(float& x, float& y, float& z) const;
    
    // Применение трансформаций OpenGL
    void ApplyTransform() const;
    
    // Сброс камеры в начальное положение
    void Reset();
    
    // Настройки чувствительности
    void SetMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
    float GetMouseSensitivity() const { return m_mouseSensitivity; }
    
    // Ограничения
    void SetRotationLimits(float minRotX, float maxRotX);
    void SetFOVLimits(float minFOV, float maxFOV);
    
private:
    // Позиция камеры
    float m_x, m_y, m_z;
    
    // Вращение камеры (в градусах)
    float m_rotX, m_rotY;
    
    // Поле зрения (в градусах)
    float m_fov;
    float m_targetFOV;        // Целевой FOV для плавного перехода
    float m_fovVelocity;      // Скорость изменения FOV для инертности
    
    // Целевые позиции и вращения для плавного движения
    float m_targetX, m_targetY, m_targetZ;
    float m_targetRotX, m_targetRotY;
    float m_posVelocityX, m_posVelocityY, m_posVelocityZ;
    
    // Чувствительность мыши
    float m_mouseSensitivity;
    
    // Ограничения
    float m_minRotX, m_maxRotX;
    float m_minFOV, m_maxFOV;
    
    // Начальные значения для сброса
    float m_initialX, m_initialY, m_initialZ;
    float m_initialRotX, m_initialRotY;
    float m_initialFOV;
    
    // Вспомогательные методы
    void ClampRotation();
    void ClampFOV();
    float DegreesToRadians(float degrees) const;
};
