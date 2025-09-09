#include "Grid.h"
#include <iostream>

// Включаем OpenGL заголовки
#ifdef _WIN32
    #include <windows.h>
#endif

// OpenGL - используем GLEW (включен в Renderer.h)

Grid::Grid() : m_gridSize(3000.0f), m_gridSpacing(100.0f), m_gridSegments(60) {
}

Grid::~Grid() {
    Shutdown();
}

bool Grid::Initialize() {
    // Для базового OpenGL 1.1 используем простую сетку без шейдеров
    std::cout << "[Grid] Используется базовый рендеринг сетки OpenGL 1.1" << std::endl;
    
    // Генерируем сетку
    GenerateGrid();
    
    // Настраиваем буферы (используем базовые функции)
    SetupBuffers();
    
    return true;
}

void Grid::GenerateGrid() {
    m_vertices.clear();
    m_indices.clear();
    
    const float halfSize = m_gridSize * 0.5f;
    const float spacing = m_gridSpacing;
    const int segments = m_gridSegments;
    
    // Генерируем горизонтальные линии (параллельно оси X)
    for (int i = 0; i <= segments; ++i) {
        const float y = -halfSize + i * spacing;
        const float color = (i % 5 == 0) ? 0.5f : 0.3f;
        
        // Линия от -X до +X
        m_vertices.push_back({-halfSize, y, 0.0f, color, color, color, 1.0f});
        m_vertices.push_back({halfSize, y, 0.0f, color, color, color, 1.0f});
    }
    
    // Генерируем вертикальные линии (параллельно оси Y)
    for (int i = 0; i <= segments; ++i) {
        const float x = -halfSize + i * spacing;
        const float color = (i % 5 == 0) ? 0.5f : 0.3f;
        
        // Линия от -Y до +Y
        m_vertices.push_back({x, -halfSize, 0.0f, color, color, color, 1.0f});
        m_vertices.push_back({x, halfSize, 0.0f, color, color, color, 1.0f});
    }
    
    // Генерируем индексы для линий
    m_indices.reserve(m_vertices.size());
    for (size_t i = 0; i < m_vertices.size(); i += 2) {
        m_indices.push_back(i);
        m_indices.push_back(i + 1);
    }
    
    std::cout << "[Grid] Сетка GTA SA: " << m_vertices.size() << " вершин, " << m_gridSize << "м, интервал " << m_gridSpacing << "м" << std::endl;
}

void Grid::SetupBuffers() {
    // В базовом OpenGL 1.1 используем display lists вместо VBO/VAO
    // Просто сохраняем данные в векторах для рендеринга
}

void Grid::Render(float gridScale) {
    if (m_vertices.empty()) return;
    
    // В OpenGL 3.3 Core Profile используем современный подход
    // Пока что оставляем заглушку - сетка будет переписана на VBO/VAO
    // TODO: Переписать на современный OpenGL с использованием VBO/VAO для сетки
}

void Grid::Shutdown() {
    // В базовом OpenGL 1.1 ничего не нужно очищать
    m_vertices.clear();
    m_indices.clear();
}
