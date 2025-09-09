#include "Camera.h"
// OpenGL - используем GLEW (включен в Renderer.h)

Camera::Camera() : m_x(0.0f), m_y(0.0f), m_z(0.0f),
    m_rotX(0.0f), m_rotY(0.0f), m_fov(90.0f), m_targetFOV(90.0f), m_fovVelocity(0.0f),
    m_targetX(0.0f), m_targetY(0.0f), m_targetZ(0.0f), m_targetRotX(0.0f), m_targetRotY(0.0f),
    m_posVelocityX(0.0f), m_posVelocityY(0.0f), m_posVelocityZ(0.0f),
    m_mouseSensitivity(0.1f),
    m_minRotX(-89.0f), m_maxRotX(89.0f),
    m_minFOV(8.0f), m_maxFOV(110.0f),
    m_initialX(0.0f), m_initialY(0.0f), m_initialZ(0.0f),
    m_initialRotX(0.0f), m_initialRotY(0.0f), m_initialFOV(90.0f) {
}

Camera::~Camera() {
}

void Camera::Initialize(float x, float y, float z, float rotX, float rotY, float fov) {
    m_x = x;
    m_y = y;
    m_z = z;
    m_rotX = rotX;
    m_rotY = rotY;
    m_fov = fov;
    m_targetFOV = fov;
    m_fovVelocity = 0.0f;
    
    // Инициализируем целевые позиции и вращения
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
    m_targetRotX = rotX;
    m_targetRotY = rotY;
    m_posVelocityX = 0.0f;
    m_posVelocityY = 0.0f;
    m_posVelocityZ = 0.0f;
    
    // Сохраняем начальные значения для сброса
    m_initialX = x;
    m_initialY = y;
    m_initialZ = z;
    m_initialRotX = rotX;
    m_initialRotY = rotY;
    m_initialFOV = fov;
    
    // Применяем ограничения
    ClampRotation();
    ClampFOV();
}

void Camera::SetPosition(float x, float y, float z) {
    m_x = x;
    m_y = y;
    m_z = z;
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
    m_posVelocityX = 0.0f;
    m_posVelocityY = 0.0f;
    m_posVelocityZ = 0.0f;
}

void Camera::SetRotation(float rotX, float rotY) {
    m_rotX = rotX;
    m_rotY = rotY;
    m_targetRotX = rotX;
    m_targetRotY = rotY;
    ClampRotation();
}

void Camera::SetFOV(float fov) {
    m_fov = fov;
    m_targetFOV = fov;
    m_fovVelocity = 0.0f;
    ClampFOV();
}

void Camera::MoveForward(float distance) {
    // Устаревший метод - используем MoveForwardSmooth
    MoveForwardSmooth(distance);
}

void Camera::MoveRight(float distance) {
    // Устаревший метод - используем MoveRightSmooth
    MoveRightSmooth(distance);
}

void Camera::MoveUp(float distance) {
    // Устаревший метод - используем MoveUpSmooth
    MoveUpSmooth(distance);
}

void Camera::Rotate(float deltaX, float deltaY) {
    // Устаревший метод - используем RotateSmooth
    RotateSmooth(deltaX, deltaY);
}

// Новые методы с инертностью
void Camera::MoveForwardSmooth(float distance) {
    float radY = DegreesToRadians(m_targetRotY);
    m_targetX += sin(radY) * distance;
    m_targetY += cos(radY) * distance;
}

void Camera::MoveRightSmooth(float distance) {
    float radY = DegreesToRadians(m_targetRotY);
    m_targetX += cos(radY) * distance;
    m_targetY -= sin(radY) * distance;
}

void Camera::MoveUpSmooth(float distance) {
    m_targetZ += distance;
}

void Camera::RotateSmooth(float deltaX, float deltaY) {
    // Уменьшаем чувствительность горизонтального вращения (влево-вправо)
    m_targetRotY += deltaX * m_mouseSensitivity * 0.5f;  // Уменьшили в 2 раза
    m_targetRotX += deltaY * m_mouseSensitivity;          // Вертикальное вращение оставляем как есть
    
    //// Ограничиваем целевые вращения
    //if (m_targetRotX > m_maxRotX) m_targetRotX = m_maxRotX;
    //if (m_targetRotX < m_minRotX) m_targetRotX = m_minRotX;
    
    //// Умная нормализация горизонтального вращения - используем кратчайший путь
    //// Это предотвращает резкие скачки при переходе через 360°/0°
    //if (m_targetRotY > 360.0f) {
    //    m_targetRotY -= 360.0f;
    //    // Если текущий угол близок к 0°, то целевой тоже приводим к 0°
    //    if (m_rotY < 180.0f) {
    //        m_targetRotY = 360.0f;
    //    }
    //}
    //if (m_targetRotY < 0.0f) {
    //    m_targetRotY += 360.0f;
    //    // Если текущий угол близок к 360°, то целевой тоже приводим к 360°
    //    if (m_rotY > 180.0f) {
    //        m_targetRotY = 360.0f;
    //    }
    //}
}

void Camera::UpdateFOV(float delta) {
    // Обновляем целевой FOV вместо текущего
    // Увеличиваем шаг прокрутки в 2 раза (было 2.0f, стало 4.0f)
    m_targetFOV -= delta * 4.0f; // Чувствительность прокрутки
    
    // Ограничиваем целевой FOV
    if (m_targetFOV > m_maxFOV) m_targetFOV = m_maxFOV;
    if (m_targetFOV < m_minFOV) m_targetFOV = m_minFOV;
    
    // Сбрасываем скорость для плавного старта
    m_fovVelocity = 0.0f;
}

void Camera::GetForwardVector(float& x, float& y, float& z) const {
    float radX = DegreesToRadians(m_rotX);
    float radY = DegreesToRadians(m_rotY);
    
    x = sin(radY) * cos(radX);
    y = cos(radY) * cos(radX);
    z = sin(radX);
}

void Camera::GetRightVector(float& x, float& y, float& z) const {
    float radY = DegreesToRadians(m_rotY);
    
    x = cos(radY);
    y = -sin(radY);
    z = 0.0f;
}

void Camera::GetUpVector(float& x, float& y, float& z) const {
    float radX = DegreesToRadians(m_rotX);
    float radY = DegreesToRadians(m_rotY);
    
    x = -sin(radY) * sin(radX);
    y = -cos(radY) * sin(radX);
    z = cos(radX);
}

void Camera::ApplyTransform() const {
    // В OpenGL 3.3 Core Profile используем современный подход
    // Пока что оставляем заглушку - трансформации будут применены через шейдеры
    // TODO: Переписать на современный OpenGL с использованием матриц и шейдеров
}

void Camera::Reset() {
    m_x = m_initialX;
    m_y = m_initialY;
    m_z = m_initialZ;
    m_rotX = m_initialRotX;
    m_rotY = m_initialRotY;
    m_fov = m_initialFOV;
    m_targetFOV = m_initialFOV;
    m_fovVelocity = 0.0f;
    
    // Сбрасываем целевые позиции и вращения
    m_targetX = m_initialX;
    m_targetY = m_initialY;
    m_targetZ = m_initialZ;
    m_targetRotX = m_initialRotX;
    m_targetRotY = m_initialRotY;
    m_posVelocityX = 0.0f;
    m_posVelocityY = 0.0f;
    m_posVelocityZ = 0.0f;
}

void Camera::SetRotationLimits(float minRotX, float maxRotX) {
    m_minRotX = maxRotX;
    m_maxRotX = minRotX;
    ClampRotation();
}

void Camera::SetFOVLimits(float minFOV, float maxFOV) {
    m_minFOV = minFOV;
    m_maxFOV = maxFOV;
    ClampFOV();
}

void Camera::ClampRotation() {
    // Ограничиваем вертикальное вращение
    if (m_rotX > m_maxRotX) m_rotX = m_maxRotX;
    if (m_rotX < m_minRotX) m_rotX = m_minRotX;
    
    // Умная нормализация горизонтального вращения - используем кратчайший путь
    if (m_rotY > 360.0f) m_rotY -= 360.0f;
    if (m_rotY < 0.0f) m_rotY += 360.0f;
}

void Camera::ClampFOV() {
    if (m_fov > m_maxFOV) m_fov = m_maxFOV;
    if (m_fov < m_minFOV) m_fov = m_minFOV;
}

float Camera::DegreesToRadians(float degrees) const {
    return degrees * 3.14159f / 180.0f;
}

void Camera::UpdateFOVInertia() {
    // Плавный переход к целевому FOV с инертностью
    // Используем фиксированный шаг времени для стабильности
    const float fixedTimeStep = 1.0f / 60.0f;  // Фиксированный шаг 60 FPS
    const float springStrength = 12.0f;         // Сила пружины (скорость возврата)
    const float damping = 0.8f;                 // Затухание (сопротивление)
    
    // Вычисляем разность между текущим и целевым FOV
    float fovDifference = m_targetFOV - m_fov;
    
    // Применяем пружинную физику с фиксированным временем
    m_fovVelocity += fovDifference * springStrength * fixedTimeStep;
    m_fovVelocity *= damping; // Затухание
    
    // Обновляем текущий FOV с фиксированным временем
    m_fov += m_fovVelocity * fixedTimeStep;
    
    // Применяем ограничения к текущему FOV
    ClampFOV();
}

void Camera::UpdateMovementInertia() {
    // Плавный переход только к целевым позициям (без инертности вращения)
    const float fixedTimeStep = 1.0f / 60.0f;  // Фиксированный шаг 60 FPS
    const float posSpringStrength = 8.0f;       // Сила пружины для позиции
    const float posDamping = 0.85f;             // Затухание для позиции
    
    // Позиция X
    float posDiffX = m_targetX - m_x;
    m_posVelocityX += posDiffX * posSpringStrength * fixedTimeStep;
    m_posVelocityX *= posDamping;
    m_x += m_posVelocityX * fixedTimeStep;
    
    // Позиция Y
    float posDiffY = m_targetY - m_y;
    m_posVelocityY += posDiffY * posSpringStrength * fixedTimeStep;
    m_posVelocityY *= posDamping;
    m_y += m_posVelocityY * fixedTimeStep;
    
    // Позиция Z
    float posDiffZ = m_targetZ - m_z;
    m_posVelocityZ += posDiffZ * posSpringStrength * fixedTimeStep;
    m_posVelocityZ *= posDamping;
    m_z += m_posVelocityZ * fixedTimeStep;
    
    // Вращение - применяем мгновенно без инертности
    m_rotX = m_targetRotX;
    m_rotY = m_targetRotY;
    
    // Применяем ограничения к текущим значениям
    ClampRotation();
}
