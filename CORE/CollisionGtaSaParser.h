#ifndef COLLISION_GTASA_PARSER_H
#define COLLISION_GTASA_PARSER_H

#include "helper.h"
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>

// Структуры на основе анализа col3Importv1.02.ms скрипта

// Заголовок COL3 файла
struct Col3Header {
    char fourCC[4];        // "COL3" - тип файла
    uint32_t fileSize;     // Размер файла (относительно после FourCC)
    char name[22];         // Имя коллизии (22 символа)
    int16_t modelId;       // ID модели
    float boundingBox[10]; // Bounding box параметры
    uint16_t sphereCount;  // Количество коллизионных сфер
    uint16_t boxCount;     // Количество коллизионных боксов
    uint16_t faceCount;    // Количество граней коллизионной сетки
    uint8_t lineCount;     // Количество линий
    uint8_t unused;        // Неиспользуемый байт
    uint32_t flags;        // Флаги
    uint32_t sphereOffset; // Смещение к коллизионным сферам
    uint32_t boxOffset;    // Смещение к коллизионным боксам
    uint32_t lineOffset;   // Смещение к линиям
    uint32_t vertexOffset; // Смещение к вершинам коллизионной сетки
    uint32_t faceOffset;   // Смещение к граням коллизионной сетки
    uint32_t planeOffset;  // Смещение к плоскостям
    uint32_t shadowFaceCount;    // Количество граней shadow mesh
    uint32_t shadowVertexOffset; // Смещение к вершинам shadow mesh
    uint32_t shadowFaceOffset;   // Смещение к граням shadow mesh
};

// Коллизионная сфера
struct CollisionSphere {
    float x, y, z;         // Позиция центра
    float radius;           // Радиус
    uint32_t material;      // Материал (hex формат)
};

// Коллизионный бокс
struct CollisionBox {
    float minX, minY, minZ; // Минимальные координаты
    float maxX, maxY, maxZ; // Максимальные координаты
    uint32_t material;      // Материал (hex формат)
};

// Вершина коллизионной сетки (16-битные координаты)
struct CollisionVertex {
    int16_t x, y, z;       // 16-битные координаты для COL3
    
    CollisionVertex() : x(0), y(0), z(0) {}
    CollisionVertex(int16_t px, int16_t py, int16_t pz) : x(px), y(py), z(pz) {}
    
    // Конвертация в float для совместимости с рендером (делим на 128.0 как в скрипте)
    float getX() const { return static_cast<float>(x) / 128.0f; }
    float getY() const { return static_cast<float>(y) / 128.0f; }
    float getZ() const { return static_cast<float>(z) / 128.0f; }
};

// Грань коллизионной сетки
struct CollisionFace {
    uint16_t a, b, c;      // Индексы вершин (как в скрипте)
    uint16_t material;      // Материал (hex формат)
    
    CollisionFace() : a(0), b(0), c(0), material(0) {}
    CollisionFace(uint16_t va, uint16_t vb, uint16_t vc, uint16_t mat) 
        : a(va), b(vb), c(vc), material(mat) {}
};

// Shadow mesh грань
struct ShadowFace {
    uint16_t a, b, c;      // Индексы вершин
    uint16_t material;      // Материал (hex формат)
    
    ShadowFace() : a(0), b(0), c(0), material(0) {}
    ShadowFace(uint16_t va, uint16_t vb, uint16_t vc, uint16_t mat) 
        : a(va), b(vb), c(vc), material(mat) {}
};

// Полная коллизионная модель
struct CollisionModel {
    std::string name;                    // Имя модели
    int16_t modelId;                     // ID модели
    
    // Bounding box и sphere
    float minX, minY, minZ;              // Минимальные координаты
    float maxX, maxY, maxZ;              // Максимальные координаты
    float centerX, centerY, centerZ;     // Центр
    float radius;                        // Радиус
    
    // Коллизионные объекты
    std::vector<CollisionSphere> spheres;
    std::vector<CollisionBox> boxes;
    
    // Коллизионная сетка
    std::vector<CollisionVertex> vertices;
    std::vector<CollisionFace> faces;
    
    // Shadow mesh
    std::vector<CollisionVertex> shadowVertices;
    std::vector<ShadowFace> shadowFaces;
    
    // Флаги
    uint32_t flags;
    
    // Конструктор
    CollisionModel() : modelId(0), minX(0.0f), minY(0.0f), minZ(0.0f),
                      maxX(0.0f), maxY(0.0f), maxZ(0.0f),
                      centerX(0.0f), centerY(0.0f), centerZ(0.0f),
                      radius(0.0f), flags(0) {}
    
    // Методы для получения количества элементов
    size_t getVertexCount() const { return vertices.size(); }
    size_t getFaceCount() const { return faces.size(); }
    size_t getSphereCount() const { return spheres.size(); }
    size_t getBoxCount() const { return boxes.size(); }
    size_t getShadowVertexCount() const { return shadowVertices.size(); }
    size_t getShadowFaceCount() const { return shadowFaces.size(); }
    
    // Проверка наличия геометрии
    bool hasMesh() const { return !faces.empty(); }
    bool hasShadowMesh() const { return !shadowFaces.empty(); }
    bool hasSpheres() const { return !spheres.empty(); }
    bool hasBoxes() const { return !boxes.empty(); }
};

// Флаги для анализа
namespace ColFlags {
    const uint32_t NOT_EMPTY = (1 << 2);     // Бит 2: есть коллизионная сетка
    const uint32_t FACE_GROUP = (1 << 4);    // Бит 4: есть группы граней
    const uint32_t SHADOW_MESH = (1 << 5);   // Бит 5: есть shadow mesh
}

// Функции для парсинга
bool loadColFile(const char* filePath, std::vector<CollisionModel>& models);
bool parseCol3Header(std::ifstream& file, Col3Header& header);
bool parseCollisionSpheres(std::ifstream& file, CollisionModel& model, uint32_t offset, uint16_t count);
bool parseCollisionBoxes(std::ifstream& file, CollisionModel& model, uint32_t offset, uint16_t count);
bool parseCollisionMesh(std::ifstream& file, CollisionModel& model, uint32_t vertexOffset, uint32_t faceOffset, uint16_t faceCount);
bool parseShadowMesh(std::ifstream& file, CollisionModel& model, uint32_t vertexOffset, uint32_t faceOffset, uint16_t faceCount);

class col {
public:
    // Функция для загрузки COL файла и подсчета COL3 секций
    static bool loadColFile(const char* filePath);
    
    // Функция для поиска COL3 секций в файле (как в скрипте 3ds Max)
    static int findCol3Sections(const char* filePath);
    
    // Функция для логирования в ImGui
    static void LogCol(const char* format, ...);
    
    // === НОВЫЕ ФУНКЦИИ ДЛЯ РАСПАКОВКИ ===
    
    // Определение типа .col файла по FourCC
    enum class ColFileType {
        SINGLE_COLLISION,    // Одиночный collision model (COLL, COL2, COL4)
        COL3_CONTAINER,      // Контейнер с множественными .col файлами (COL3)
        UNKNOWN              // Неизвестный формат
    };
    
    // Структура для информации о .col файле
    struct ColFileInfo {
        std::string filePath;        // Путь к файлу
        ColFileType type;            // Тип файла
        std::string fourCC;          // FourCC сигнатура
        size_t fileSize;             // Размер файла в байтах
        std::string modelName;       // Имя модели (если одиночный)
        int16_t modelId;             // ID модели (если одиночный)
        
        ColFileInfo() : type(ColFileType::UNKNOWN), fileSize(0), modelId(0) {}
    };
    
    // Функция для определения типа .col файла
    static ColFileType detectColFileType(const std::string& filePath);
    
    // Функция для получения информации о .col файле
    static bool getColFileInfo(const std::string& filePath, ColFileInfo& info);
    
    // Функция для распаковки .col файла в папку unpack_col
    static bool unpackColFile(const std::string& filePath, const std::string& outputDir);
    
    // Функция для распаковки всех .col файлов из папки col в unpack_col
    static bool unpackAllColFiles(const std::string& colDir, const std::string& unpackDir);
    
    // Функция для поиска всех .col файлов в папке
    static std::vector<std::string> findColFilesInDirectory(const std::string& directory);
    
    // Функция для создания папки, если её нет
    static bool createDirectoryIfNotExists(const std::string& path);
    
private:
    // Константа для поиска COL3 секций (0x334C4F43 = "COL3" в little-endian)
    static const uint32_t COL3_MAGIC = 0x334C4F43;
    
    // Вспомогательные функции для распаковки
    static bool isCol3Container(const std::string& filePath);
    static bool extractCol3Container(const std::string& filePath, const std::string& outputDir);
    static bool copySingleColFile(const std::string& filePath, const std::string& outputDir);
};

#endif 
