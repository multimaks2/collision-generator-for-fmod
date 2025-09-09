#ifndef LOADER_H
#define LOADER_H

#include <vector>
#include <string>
#include <map>
#include <cstdint>

// Перечисление типов данных в gta.dat
enum class DataType {
    IMG,
    IPL,
    IDE,
    TEXDICTION,
    MODELFILE,
    SPLASH,
    HIERFILE,
    MAPZONE,
    WATER,
    IMGLIST,
    CDIMAGE,
    EXIT,
    UNKNOWN
};

// Структура для хранения записи из gta.dat
struct GtaDatEntry {
    DataType type;
    std::string path;
    std::string additionalParam; // Для некоторых типов
    
    GtaDatEntry(DataType t, const std::string& p, const std::string& param = "") 
        : type(t), path(p), additionalParam(param) {}
};

// Класс для хранения всех данных из gta.dat
class GtaDatData {
private:
    std::vector<GtaDatEntry> entries;

public:
    // Добавить запись
    void addEntry(const GtaDatEntry& entry) { entries.push_back(entry); }
    
    // Получить все записи определенного типа
    std::vector<GtaDatEntry> getEntriesByType(DataType type) const {
        std::vector<GtaDatEntry> result;
        for (const auto& entry : entries) {
            if (entry.type == type) {
                result.push_back(entry);
            }
        }
        return result;
    }
    
    // Получить все записи
    const std::vector<GtaDatEntry>& getAllEntries() const { return entries; }
    
    // Очистить все записи
    void clear() { entries.clear(); }
};

class dat {
public:
    // Функция для загрузки файла по указанному пути
    static bool load(const char* filePath);
    
    // Функция для загрузки и парсинга gta.dat файла
    static bool loadGtaDat(const char* filePath, GtaDatData& gtaData);
    
private:
    // Вспомогательная функция для определения типа данных по ключевому слову
    static DataType parseDataType(const std::string& keyword);
};

// Класс для работы с IPL файлами
class ipl {
public:
    // Структура для хранения объекта из IPL файла (секция INST)
    struct IplObject {
        int modelId;
        std::string name;
        int interior;
        float x, y, z;
        float rx, ry, rz, rw; // rotation (quaternion)
        int lod;
        
        IplObject(int id, const std::string& objName, int interiorId, 
                  float px, float py, float pz, 
                  float rotx, float roty, float rotz, float rotw, int lodId)
            : modelId(id), name(objName), interior(interiorId), 
              x(px), y(py), z(pz), rx(rotx), ry(roty), rz(rotz), rw(rotw), lod(lodId) {}
    };
    
    // Класс для хранения всех данных из IPL файла
    class IplData {
    private:
        std::vector<IplObject> objects;
        std::string fileName;
        
    public:
        // Добавить объект
        void addObject(const IplObject& obj) { objects.push_back(obj); }
        
        // Получить все объекты
        const std::vector<IplObject>& getAllObjects() const { return objects; }
        
        // Получить количество объектов
        size_t getObjectCount() const { return objects.size(); }
        
        // Установить имя файла
        void setFileName(const std::string& name) { fileName = name; }
        
        // Получить имя файла
        const std::string& getFileName() const { return fileName; }
        
        // Очистить все данные
        void clear() { objects.clear(); }
    };
    
    // Функция для загрузки и парсинга IPL файла
    static bool loadIplFile(const char* filePath, IplData& iplData);
    
    // Функция для поиска объектов по ID модели
    static std::vector<IplObject> findObjectsByModelId(const IplData& iplData, int modelId);
    
    // Функция для поиска объектов по имени
    static std::vector<IplObject> findObjectsByName(const IplData& iplData, const std::string& name);

private:
    // Вспомогательная функция для парсинга секции INST
    static bool parseInstSection(std::ifstream& file, IplData& iplData);
    
    // Вспомогательная функция для парсинга строки INST
    static bool parseInstLine(const std::string& line, IplObject& object);
};

// Класс для работы с IMG файлами (GTA SA - IMG v2)
class img {
public:
    // Перечисление версий IMG для совместимости
    enum class ImgVersion {
        V1_GTA3_VC,     // GTA III & Vice City (.dir + .img)
        V2_GTA_SA,      // GTA San Andreas (стандартный)
        V3_GTA_IV,      // GTA IV (новый формат)
        V2_EXTENDED_SA  // GTA SA с расширенными лимитами (FLA)
    };

    // Структура заголовка IMG файла (GTA SA - IMG v2)
    struct ImgHeader {
        char magic[4];           // Магическое число "VER2"
        uint32_t fileCount;      // Количество файлов в архиве
    };
    
    // Расширенная структура заголовка для больших файлов
    struct ImgHeaderExtended {
        char magic[4];           // Магическое число "VER2" или "VER3"
        uint32_t fileCount;      // Количество файлов в архиве
        uint32_t flags;          // Флаги (0 = стандартный, 1 = расширенный)
        uint32_t reserved;       // Зарезервировано для будущего использования
    };
    
    // Стандартная структура записи файла в IMG (GTA SA - IMG v2)
    struct ImgFileEntry {
        uint32_t offset;         // Смещение файла в секторах (×2048 для байт)
        uint16_t streamingSize;  // Размер файла в секторах (×2048 для байт)
        uint16_t archiveSize;    // Размер в архиве (всегда 0)
        char name[24];           // Имя файла (null-terminated)
    };
    
    // Расширенная структура записи для больших файлов (>2GB)
    struct ImgFileEntryExtended {
        uint64_t offset;         // 64-битное смещение файла в секторах
        uint64_t streamingSize;  // 64-битный размер файла в секторах
        uint32_t archiveSize;    // Размер в архиве (всегда 0)
        char name[24];           // Имя файла (null-terminated)
    };
    
    // Универсальная структура для хранения файла из IMG
    struct ImgFile {
        std::string name;                    // Имя файла (например: "car1.dff")
        std::vector<uint8_t> data;           // Буфер с данными файла
        size_t offset;                       // Позиция в IMG файле
        size_t size;                         // Размер файла
        std::string extension;               // Расширение файла (.dff, .txd, etc.)
        ImgVersion version;                  // Версия IMG, из которой извлечен файл
        
        ImgFile() : offset(0), size(0), version(ImgVersion::V2_GTA_SA) {}
        
        ImgFile(const std::string& fileName, const std::vector<uint8_t>& fileData, 
                size_t fileOffset, size_t fileSize, ImgVersion imgVersion = ImgVersion::V2_GTA_SA)
            : name(fileName), data(fileData), offset(fileOffset), size(fileSize), version(imgVersion) {
            // Извлекаем расширение
            size_t dotPos = fileName.find_last_of('.');
            if (dotPos != std::string::npos) {
                extension = fileName.substr(dotPos);
            }
        }
    };
    
    // Класс для управления IMG файлами
    class ImgData {
    private:
        std::string fileName;                    // Путь к IMG файлу
        ImgHeader header;                        // Заголовок IMG (стандартный)
        ImgHeaderExtended headerExtended;        // Расширенный заголовок
        std::vector<ImgFileEntry> fileEntries;   // Стандартные записи файлов
        std::vector<ImgFileEntryExtended> fileEntriesExtended; // Расширенные записи
        std::map<std::string, ImgFile> files;    // Быстрый доступ к файлам по имени
        ImgVersion version;                      // Версия IMG файла
        bool isExtended;                         // Флаг расширенного формата
        
    public:
        // Конструктор
        ImgData() : header{0, 0, 0, 0}, headerExtended{0, 0, 0, 0}, 
                    version(ImgVersion::V2_GTA_SA), isExtended(false) {}
        
        // Загрузить IMG файл (автоопределение версии)
        bool loadImgFile(const char* filePath);
        
        // Принудительная загрузка определенной версии
        bool loadImgFileVersion(const char* filePath, ImgVersion forcedVersion);
        
        // Получить версию IMG файла
        ImgVersion getVersion() const { return version; }
        
        // Проверить, является ли файл расширенным
        bool isExtendedFormat() const { return isExtended; }
        
        // Получить список всех имен файлов
        std::vector<std::string> getAllFileNames() const;
        
        // Получить файл по имени
        const ImgFile* getFile(const std::string& fileName) const;
        
        // Проверить существование файла
        bool fileExists(const std::string& fileName) const;
        
        // Получить количество файлов
        size_t getFileCount() const { 
            return isExtended ? headerExtended.fileCount : header.fileCount; 
        }
        
        // Получить имя IMG файла
        const std::string& getFileName() const { return fileName; }
        
        // Получить все файлы определенного типа (например, только .dff)
        std::vector<ImgFile> getFilesByExtension(const std::string& ext) const;
        
        // Очистить все данные
        void clear();
        
        // Получить информацию о размерах файлов
        uint64_t getTotalSizeInBytes() const;
        uint64_t getLargestFileSizeInBytes() const;
        
    private:
        // Вспомогательные методы
        bool parseHeader(std::ifstream& file);
        bool parseFileEntries(std::ifstream& file, const std::string& imgFileName = "");
        bool loadFileData(std::ifstream& file, ImgFileEntry& entry, ImgFile& imgFile, const std::string& imgFileName = "");
        bool loadFileDataExtended(std::ifstream& file, ImgFileEntryExtended& entry, ImgFile& imgFile, const std::string& imgFileName = "");
        
        // Автоопределение версии IMG
        ImgVersion detectImgVersion(std::ifstream& file);
        
        // Конвертация расширенных записей в стандартные (для совместимости)
        ImgFileEntry convertToStandardEntry(const ImgFileEntryExtended& extendedEntry) const;
    };
    
    // Статические функции для работы с IMG
    static bool loadImgFile(const char* filePath, ImgData& imgData);
    static std::vector<ImgFile> findFilesByExtension(const ImgData& imgData, const std::string& extension);
    static std::vector<ImgFile> findFilesByNamePattern(const ImgData& imgData, const std::string& pattern);
    
    // Новые функции для поиска моделей по названию
    static const ImgFile* findModelByName(const std::vector<ImgData*>& imgArchives, const std::string& modelName);
    static std::vector<uint8_t> getModelData(const std::vector<ImgData*>& imgArchives, const std::string& modelName);
    static bool modelExists(const std::vector<ImgData*>& imgArchives, const std::string& modelName);
    
    // Функция для автопоиска .img файлов в папке models
    static std::vector<std::string> findImgFilesInModelsFolder();
    
    // Утилиты для работы с большими файлами
    static bool isLargeFileSupported(const ImgData& imgData);
    static std::string getVersionString(ImgVersion version);
    static uint64_t sectorsToBytes(uint64_t sectors);
    static uint64_t bytesToSectors(uint64_t bytes);
};

// Класс для работы с DFF файлами (RenderWare Data File) - только геометрия
class dff {
public:
    // Структура для хранения вершины
    struct Vertex {
        float x, y, z;           // Позиция вершины
        
        Vertex() : x(0.0f), y(0.0f), z(0.0f) {}
        Vertex(float px, float py, float pz) : x(px), y(py), z(pz) {}
    };
    
    // Структура для хранения нормали
    struct Normal {
        float x, y, z;           // Вектор нормали
        
        Normal() : x(0.0f), y(0.0f), z(0.0f) {}
        Normal(float nx, float ny, float nz) : x(nx), y(ny), z(nz) {}
    };
    
    // Структура для хранения UV координат
    struct UVCoord {
        float u, v;              // UV координаты
        
        UVCoord() : u(0.0f), v(0.0f) {}
        UVCoord(float pu, float pv) : u(pu), v(pv) {}
    };
    
    // Структура для хранения цвета вершины
    struct VertexColor {
        uint8_t r, g, b, a;      // RGBA цвет
        
        VertexColor() : r(255), g(255), b(255), a(255) {}
        VertexColor(uint8_t pr, uint8_t pg, uint8_t pb, uint8_t pa) : r(pr), g(pg), b(pb), a(pa) {}
    };
    
    // Структура для хранения полигона (треугольника)
    struct Polygon {
        uint32_t vertex1, vertex2, vertex3;  // Индексы вершин
        uint32_t materialId;                 // ID материала
        
        Polygon() : vertex1(0), vertex2(0), vertex3(0), materialId(0) {}
        Polygon(uint32_t v1, uint32_t v2, uint32_t v3, uint32_t matId = 0) 
            : vertex1(v1), vertex2(v2), vertex3(v3), materialId(matId) {}
    };
    
    // Структура для хранения материала
    struct Material {
        std::string name;
        std::string textureName;
        VertexColor color;
        float ambient, diffuse, specular;
        
        Material() : ambient(1.0f), diffuse(1.0f), specular(0.0f) {}
    };
    
    // Структура для хранения загруженной DFF модели (только геометрия)
    struct DffModel {
        std::string name;
        std::vector<Vertex> vertices;
        std::vector<Normal> normals;
        std::vector<UVCoord> uvCoords;
        std::vector<VertexColor> vertexColors;
        std::vector<Polygon> polygons;
        std::vector<Material> materials;
        uint32_t vao, vbo, ebo, normalVBO;
        
        // Конструктор по умолчанию
        DffModel() : vao(0), vbo(0), ebo(0), normalVBO(0) {}
        
        // Методы для получения количества элементов
        size_t getVertexCount() const { return vertices.size(); }
        size_t getPolygonCount() const { return polygons.size(); }
        size_t getNormalCount() const { return normals.size(); }
        size_t getMaterialCount() const { return materials.size(); }
        
        // Методы для работы с GPU
        bool loadToGPU();
        void clear();
    };
    
    // Класс для управления DFF файлами
    class DffData {
    private:
        std::string fileName;
        DffModel model;
        
        // Внутренние переменные для парсинга (как в DffImporter)
        char* buffer;
        unsigned int bufIndex;
        std::streamsize fileSize;
        bool noIssues;
        
        // Методы парсинга из DffImporter
        bool parseFile();
        bool parseClump();
        bool findAndParseGeometryList();
        bool parseFrameList();
        bool parseGeometryList();
        bool parseGeometry(const uint32_t& gIndex);
        bool parseMaterialList(const uint32_t& geoIndex);
        bool parseMaterial(const uint32_t& matID);
        bool parseTexture();
        bool parseBinMesh();
        bool parseExtension();
        
        // Вспомогательные методы
        template<typename T>
        T readBuffer(const unsigned int& inc);
        template<typename T>
        T readNumeric();
        
    public:
        DffData();
        ~DffData();
        
        bool loadDffFile(const char* filePath);
        bool loadDffFromBuffer(const std::vector<uint8_t>& data, const std::string& name = "");
        const DffModel& getModel() const { return model; }
        bool uploadToGPU();
        void clear();
    };
    
    // Статические функции для работы с DFF
    static bool loadDffFile(const char* filePath, DffData& dffData);
    static bool loadDffFromBuffer(const std::vector<uint8_t>& data, DffData& dffData, const std::string& name = "");
    static DffModel* createDffModel(const std::vector<uint8_t>& data, const std::string& name = "");
    static bool createTestDffFile(const char* filePath);
};

#endif // LOADER_H
