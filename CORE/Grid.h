#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

// Структура для вершины сетки
struct GridVertex {
    float x, y, z;      // Позиция
    float r, g, b, a;   // Цвет
};

class Grid {
public:
    Grid();
    ~Grid();
    
    bool Initialize();
    void Render(float gridScale = 1.0f);
    void Shutdown();
    
private:
    // Данные сетки
    std::vector<GridVertex> m_vertices;
    std::vector<unsigned int> m_indices;
    
    // Параметры сетки
    float m_gridSize;
    float m_gridSpacing;
    int m_gridSegments;
    
    // Методы
    void GenerateGrid();
    void SetupBuffers();
};
