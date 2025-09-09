#include "Loader.h"
#include "Logger.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <GL/glew.h>
#include <vector>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

bool dat::load(const char* filePath) {
    if (!filePath) {
        LogError("Loader: Путь к файлу пуст");
        return false;
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LogError("Loader: Не удалось открыть файл: " + std::string(filePath));
        return false;
    }
    
    file.close();
    return true;
}

bool dat::loadGtaDat(const char* filePath, GtaDatData& gtaData) {
    if (!filePath) {
        LogError("Loader: Путь к файлу пуст");
        return false;
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LogError("Loader: Не удалось открыть файл: " + std::string(filePath));
        return false;
    }
    
    gtaData.clear();
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Убираем лишние пробелы в начале и конце
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;
        
        // Преобразуем в верхний регистр для сравнения
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);
        
        DataType type = parseDataType(keyword);
        if (type == DataType::UNKNOWN) {
            LogWarning("Loader: Неизвестный тип данных '" + keyword + "' на строке " + std::to_string(lineNumber));
            continue;
        }
        
        std::string path;
        std::string additionalParam;
        
        iss >> path;
        
        if (!path.empty()) {
            gtaData.addEntry(GtaDatEntry(type, path, additionalParam));
        }
    }
    
    file.close();
    return true;
}

DataType dat::parseDataType(const std::string& keyword) {
    if (keyword == "IMG") return DataType::IMG;
    if (keyword == "IPL") return DataType::IPL;
    if (keyword == "IDE") return DataType::IDE;
    if (keyword == "TEXDICTION") return DataType::TEXDICTION;
    if (keyword == "MODELFILE") return DataType::MODELFILE;
    if (keyword == "SPLASH") return DataType::SPLASH;
    if (keyword == "HIERFILE") return DataType::HIERFILE;
    if (keyword == "MAPZONE") return DataType::MAPZONE;
    if (keyword == "WATER") return DataType::WATER;
    if (keyword == "IMGLIST") return DataType::IMGLIST;
    if (keyword == "CDIMAGE") return DataType::CDIMAGE;
    if (keyword == "EXIT") return DataType::EXIT;
    
    return DataType::UNKNOWN;
}

// Реализация класса ipl
bool ipl::loadIplFile(const char* filePath, IplData& iplData) {
    if (!filePath) {
        LogError("IPL: Путь к файлу пуст");
        return false;
    }
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LogError("IPL: Не удалось открыть файл: " + std::string(filePath));
        return false;
    }
    
    iplData.clear();
    iplData.setFileName(filePath);
    
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Убираем лишние пробелы в начале и конце
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        // Проверяем начало секции INST
        if (line == "inst") {
            if (!parseInstSection(file, iplData)) {
                LogError("IPL: Ошибка парсинга секции INST на строке " + std::to_string(lineNumber));
            }
        }
    }
    
    file.close();
    return true;
}

bool ipl::parseInstSection(std::ifstream& file, IplData& iplData) {
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Убираем лишние пробелы
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Проверяем конец секции
        if (line == "end") {
            return true;
        }
        
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Парсим строку INST
        IplObject object(0, "", 0, 0, 0, 0, 0, 0, 0, 0, 0);
        if (parseInstLine(line, object)) {
            iplData.addObject(object);
        }
    }
    
    return false; // Не найден конец секции
}

bool ipl::parseInstLine(const std::string& line, IplObject& object) {
    std::istringstream iss(line);
    std::string token;
    
    // Формат: ID, NAME, INTERIOR, X, Y, Z, RX, RY, RZ, RW, LOD
    // Пример: 2001, u_panel_01, 0, 589.60840, 852.89099, 16.82146, 0.0, 0.0, 0.855037, 0.518567, 1
    
    // Читаем ID
    if (!(iss >> object.modelId)) return false;
    
    // Пропускаем запятую
    if (!(iss >> token) || token != ",") return false;
    
    // Читаем имя (может содержать пробелы, поэтому читаем до следующей запятой)
    std::getline(iss, object.name, ',');
    // Убираем пробелы в начале и конце имени
    object.name.erase(0, object.name.find_first_not_of(" \t"));
    object.name.erase(object.name.find_last_not_of(" \t") + 1);
    
    // Читаем INTERIOR
    if (!(iss >> object.interior)) return false;
    
    // Пропускаем запятую
    if (!(iss >> token) || token != ",") return false;
    
    // Читаем координаты X, Y, Z
    if (!(iss >> object.x)) return false;
    if (!(iss >> token) || token != ",") return false;
    if (!(iss >> object.y)) return false;
    if (!(iss >> token) || token != ",") return false;
    if (!(iss >> object.z)) return false;
    
    // Пропускаем запятую
    if (!(iss >> token) || token != ",") return false;
    
    // Читаем повороты RX, RY, RZ, RW
    if (!(iss >> object.rx)) return false;
    if (!(iss >> token) || token != ",") return false;
    if (!(iss >> object.ry)) return false;
    if (!(iss >> token) || token != ",") return false;
    if (!(iss >> object.rz)) return false;
    if (!(iss >> token) || token != ",") return false;
    if (!(iss >> object.rw)) return false;
    
    // Пропускаем запятую
    if (!(iss >> token) || token != ",") return false;
    
    // Читаем LOD
    if (!(iss >> object.lod)) return false;
    
    return true;
}

std::vector<ipl::IplObject> ipl::findObjectsByModelId(const IplData& iplData, int modelId) {
    std::vector<IplObject> result;
    for (const auto& obj : iplData.getAllObjects()) {
        if (obj.modelId == modelId) {
            result.push_back(obj);
        }
    }
    return result;
}

std::vector<ipl::IplObject> ipl::findObjectsByName(const IplData& iplData, const std::string& name) {
    std::vector<IplObject> result;
    for (const auto& obj : iplData.getAllObjects()) {
        if (obj.name == name) {
            result.push_back(obj);
        }
    }
    return result;
}

// ============================================================================
// Реализация методов для работы с IMG файлами
// ============================================================================

// Статическая функция для загрузки IMG файла
bool img::loadImgFile(const char* filePath, ImgData& imgData) {
    return imgData.loadImgFile(filePath);
}

// Метод загрузки IMG файла (автоопределение версии)
bool img::ImgData::loadImgFile(const char* filePath) {
    if (!filePath) {
        //printf("[IMG] Ошибка: Путь к файлу пуст\n");
        return false;
    }
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        //printf("[IMG] Ошибка: Не удалось открыть файл: %s\n", filePath);
        return false;
    }
    
    this->fileName = filePath;
    
    // Автоопределение версии IMG
    this->version = detectImgVersion(file);
    this->isExtended = (version == ImgVersion::V2_EXTENDED_SA);
    
    // Парсим заголовок
    if (!parseHeader(file)) {
        //printf("[IMG] Ошибка: Не удалось прочитать заголовок\n");
        file.close();
        return false;
    }
    
    // Парсим записи файлов
    if (!parseFileEntries(file, fileName)) {
        //printf("[IMG] Ошибка: Не удалось прочитать записи файлов\n");
        file.close();
        return false;
    }
    
    // Загружаем данные всех файлов
    if (isExtended) {
        // Для расширенного формата используем 64-битные записи
        for (auto& entry : fileEntriesExtended) {
            ImgFile imgFile;
            if (loadFileDataExtended(file, entry, imgFile, fileName)) {
                files[imgFile.name] = imgFile;
            }
        }
    } else {
        // Для стандартного формата используем 32-битные записи
        for (auto& entry : fileEntries) {
            ImgFile imgFile;
            if (loadFileData(file, entry, imgFile, fileName)) {
                files[imgFile.name] = imgFile;
            }
        }
    }
    
    file.close();
    return true;
}

// Принудительная загрузка определенной версии
bool img::ImgData::loadImgFileVersion(const char* filePath, ImgVersion forcedVersion) {
    if (!filePath) {
        //printf("[IMG] Ошибка: Путь к файлу пуст\n");
        return false;
    }
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        //printf("[IMG] Ошибка: Не удалось открыть файл: %s\n", filePath);
        return false;
    }
    
    this->fileName = filePath;
    this->version = forcedVersion;
    this->isExtended = (version == ImgVersion::V2_EXTENDED_SA);
    
    // Парсим заголовок
    if (!parseHeader(file)) {
        //printf("[IMG] Ошибка: Не удалось прочитать заголовок\n");
        file.close();
        return false;
    }
    
    // Парсим записи файлов
    if (!parseFileEntries(file, fileName)) {
        //printf("[IMG] Ошибка: Не удалось прочитать записи файлов\n");
        file.close();
        return false;
    }
    
    // Загружаем данные всех файлов
    if (isExtended) {
        for (auto& entry : fileEntriesExtended) {
            ImgFile imgFile;
            if (loadFileDataExtended(file, entry, imgFile, fileName)) {
                files[imgFile.name] = imgFile;
            }
        }
    } else {
        for (auto& entry : fileEntries) {
            ImgFile imgFile;
            if (loadFileData(file, entry, imgFile, fileName)) {
                files[imgFile.name] = imgFile;
            }
        }
    }
    
    file.close();
    return true;
}

// Парсинг заголовка IMG
bool img::ImgData::parseHeader(std::ifstream& file) {
    if (isExtended) {
        // Для расширенного формата читаем расширенный заголовок
        file.read(reinterpret_cast<char*>(&headerExtended), sizeof(ImgHeaderExtended));
        
        // Проверяем магическое число "VER2" или "VER3"
        if (strncmp(headerExtended.magic, "VER2", 4) != 0 && 
            strncmp(headerExtended.magic, "VER3", 4) != 0) {
            //printf("[IMG] Ошибка: Неверный формат расширенного IMG (ожидается VER2/VER3, получено: %.4s)\n", headerExtended.magic);
            return false;
        }
        
        // Проверяем флаг расширенного формата
        if (headerExtended.flags != 1) {
            //printf("[IMG] Предупреждение: Флаг расширенного формата не установлен\n");
        }
        
        //printf("[IMG] Расширенный заголовок: магия=%.4s, файлов=%u, флаги=%u\n", 
        //       headerExtended.magic, headerExtended.fileCount, headerExtended.flags);
    } else {
        // Для стандартного формата читаем обычный заголовок
        file.read(reinterpret_cast<char*>(&header), sizeof(ImgHeader));
        
        // Проверяем магическое число "VER2"
        if (strncmp(header.magic, "VER2", 4) != 0) {
            //printf("[IMG] Ошибка: Неверный формат IMG (ожидается VER2, получено: %.4s)\n", header.magic);
            return false;
        }
        
        //printf("[IMG] Стандартный заголовок: магия=%.4s, файлов=%u\n", header.magic, header.fileCount);
    }
    
    return true;
}

// Парсинг записей файлов
bool img::ImgData::parseFileEntries(std::ifstream& file, const std::string& imgFileName) {
    if (isExtended) {
        // Для расширенного формата используем 64-битные записи
        fileEntriesExtended.clear();
        fileEntriesExtended.reserve(headerExtended.fileCount);
        
        for (uint32_t i = 0; i < headerExtended.fileCount; i++) {
            ImgFileEntryExtended entry;
            file.read(reinterpret_cast<char*>(&entry), sizeof(ImgFileEntryExtended));
            
            // Проверяем корректность записи (размер в секторах)
            if (entry.streamingSize == 0) {
                // Файлы с размером 0 байт - это нормально (пустые файлы-заглушки)
                fileEntriesExtended.push_back(entry);
            } else if (entry.streamingSize > 0 && entry.streamingSize < 1000000000) { // Максимум ~2TB (1 миллиард секторов)
                fileEntriesExtended.push_back(entry);
            } else {
                if (!imgFileName.empty()) {
                    //printf("[%s] Предупреждение: Некорректный размер файла %u: %llu секторов для '%s'\n", 
                    //       imgFileName.c_str(), i, (unsigned long long)entry.streamingSize, entry.name);
                } else {
                    //printf("[IMG] Предупреждение: Некорректный размер файла %u: %llu секторов для '%s'\n", 
                    //       i, (unsigned long long)entry.streamingSize, entry.name);
                }
            }
        }
        
        //printf("[IMG] Прочитано расширенных записей файлов: %zu\n", fileEntriesExtended.size());
    } else {
        // Для стандартного формата используем 32-битные записи
        fileEntries.clear();
        fileEntries.reserve(header.fileCount);
        
        for (uint32_t i = 0; i < header.fileCount; i++) {
            ImgFileEntry entry;
            file.read(reinterpret_cast<char*>(&entry), sizeof(ImgFileEntry));
            
            // Проверяем корректность записи (размер в секторах)
            if (entry.streamingSize == 0) {
                // Файлы с размером 0 байт - это нормально (пустые файлы-заглушки)
                fileEntries.push_back(entry);
            } else if (entry.streamingSize > 0 && entry.streamingSize < 50000) { // Максимум ~100MB (50000 секторов)
                fileEntries.push_back(entry);
            } else {
                if (!imgFileName.empty()) {
                    //printf("[%s] Предупреждение: Некорректный размер файла %u: %u секторов для '%s'\n", imgFileName.c_str(), i, entry.streamingSize, entry.name);
                } else {
                    //printf("[IMG] Предупреждение: Некорректный размер файла %u: %u секторов для '%s'\n", i, entry.streamingSize, entry.name);
                }
            }
        }
        
        //printf("[IMG] Прочитано стандартных записей файлов: %zu\n", fileEntries.size());
    }
    
    return true;
}

// Загрузка данных файла
bool img::ImgData::loadFileData(std::ifstream& file, ImgFileEntry& entry, ImgFile& imgFile, const std::string& imgFileName) {
    // Конвертируем секторы в байты
    uint32_t offsetBytes = entry.offset * 2048;
    uint32_t sizeBytes = entry.streamingSize * 2048;
    
    // Специальная обработка для файлов с размером 0 байт
    if (entry.streamingSize == 0) {
        // Для пустых файлов просто создаем пустой буфер
        imgFile.data.clear();
        imgFile.name = std::string(entry.name);
        imgFile.offset = offsetBytes;
        imgFile.size = 0;
        imgFile.version = ImgVersion::V2_GTA_SA;
        
        // Извлекаем расширение
        size_t dotPos = imgFile.name.find_last_of('.');
        if (dotPos != std::string::npos) {
            imgFile.extension = imgFile.name.substr(dotPos);
        }
        
        return true;
    }
    
    // Переходим к позиции файла
    file.seekg(offsetBytes);
    
    // Проверяем, что файл не выходит за границы
    if (file.tellg() != static_cast<std::streampos>(offsetBytes)) {
        if (!imgFileName.empty()) {
            //printf("[%s] Ошибка: Не удалось перейти к позиции %u байт (сектор %u) для файла '%s'\n", imgFileName.c_str(), offsetBytes, entry.offset, entry.name);
        } else {
            //printf("[IMG] Ошибка: Не удалось перейти к позиции %u байт (сектор %u) для файла '%s'\n", offsetBytes, entry.offset, entry.name);
        }
        return false;
    }
    
    // Читаем данные файла
    imgFile.data.resize(sizeBytes);
    file.read(reinterpret_cast<char*>(imgFile.data.data()), sizeBytes);
    
    if (file.gcount() != static_cast<std::streamsize>(sizeBytes)) {
        if (!imgFileName.empty()) {
            //printf("[%s] Ошибка: Прочитано %ld байт вместо %u (секторов: %u) для файла '%s'\n", imgFileName.c_str(), file.gcount(), sizeBytes, entry.streamingSize, entry.name);
        } else {
            //printf("[IMG] Ошибка: Прочитано %ld байт вместо %u (секторов: %u) для файла '%s'\n", file.gcount(), sizeBytes, entry.streamingSize, entry.name);
        }
        return false;
    }
    
    // Заполняем остальные поля для обычных файлов
    imgFile.name = std::string(entry.name);
    imgFile.offset = offsetBytes;
    imgFile.size = sizeBytes;
    imgFile.version = ImgVersion::V2_GTA_SA;
    
    // Извлекаем расширение
    size_t dotPos = imgFile.name.find_last_of('.');
    if (dotPos != std::string::npos) {
        imgFile.extension = imgFile.name.substr(dotPos);
    }
    
    return true;
}

// Получить список всех имен файлов
std::vector<std::string> img::ImgData::getAllFileNames() const {
    std::vector<std::string> result;
    result.reserve(files.size());
    
    for (const auto& pair : files) {
        result.push_back(pair.first);
    }
    
    return result;
}

// Получить файл по имени
const img::ImgFile* img::ImgData::getFile(const std::string& fileName) const {
    auto it = files.find(fileName);
    if (it != files.end()) {
        return &(it->second);
    }
    return nullptr;
}

// Проверить существование файла
bool img::ImgData::fileExists(const std::string& fileName) const {
    return files.find(fileName) != files.end();
}

// Получить файлы по расширению
std::vector<img::ImgFile> img::ImgData::getFilesByExtension(const std::string& ext) const {
    std::vector<ImgFile> result;
    
    for (const auto& pair : files) {
        if (pair.second.extension == ext) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

// Очистить все данные
void img::ImgData::clear() {
    fileName.clear();
    memset(&header, 0, sizeof(header));
    memset(&headerExtended, 0, sizeof(headerExtended));
    fileEntries.clear();
    fileEntriesExtended.clear();
    files.clear();
    version = ImgVersion::V2_GTA_SA;
    isExtended = false;
}

// Статические функции поиска
std::vector<img::ImgFile> img::findFilesByExtension(const ImgData& imgData, const std::string& extension) {
    return imgData.getFilesByExtension(extension);
}

std::vector<img::ImgFile> img::findFilesByNamePattern(const ImgData& imgData, const std::string& pattern) {
    std::vector<ImgFile> result;
    
    for (const auto& fileName : imgData.getAllFileNames()) {
        if (fileName.find(pattern) != std::string::npos) {
            const ImgFile* file = imgData.getFile(fileName);
            if (file) {
                result.push_back(*file);
            }
        }
    }
    
    return result;
}

// Новые функции для поиска моделей по названию

// Найти модель по названию во всех IMG архивах
const img::ImgFile* img::findModelByName(const std::vector<ImgData*>& imgArchives, const std::string& modelName) {
    for (const auto& imgData : imgArchives) {
        if (imgData) {
            const ImgFile* file = imgData->getFile(modelName);
            if (file) {
                return file;
            }
        }
    }
    return nullptr;
}

// Получить данные модели по названию
std::vector<uint8_t> img::getModelData(const std::vector<ImgData*>& imgArchives, const std::string& modelName) {
    const ImgFile* file = findModelByName(imgArchives, modelName);
    if (file) {
        return file->data;
        }
    return std::vector<uint8_t>(); // Пустой вектор если модель не найдена
}

// Проверить существование модели
bool img::modelExists(const std::vector<ImgData*>& imgArchives, const std::string& modelName) {
    return findModelByName(imgArchives, modelName) != nullptr;
}

// Автопоиск .img файлов в папке models
std::vector<std::string> img::findImgFilesInModelsFolder() {
    std::vector<std::string> imgFiles;
    
    // Проверяем существование папки models
    std::string modelsPath = "models";
    
    // Ищем все .img файлы в папке models
    std::string searchPattern = modelsPath + "\\*.img";
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string fileName = findData.cFileName;
                std::string fullPath = modelsPath + "\\" + fileName;
                imgFiles.push_back(fullPath);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    return imgFiles;
}

// ============================================================================
// РАСШИРЕННАЯ ПОДДЕРЖКА БОЛЬШИХ IMG ФАЙЛОВ
// ============================================================================

// Загрузка данных файла для расширенного формата (64-битные размеры)
bool img::ImgData::loadFileDataExtended(std::ifstream& file, ImgFileEntryExtended& entry, ImgFile& imgFile, const std::string& imgFileName) {
    // Конвертируем секторы в байты (64-битные)
    uint64_t offsetBytes = entry.offset * 2048;
    uint64_t sizeBytes = entry.streamingSize * 2048;
    
    // Специальная обработка для файлов с размером 0 байт
    if (entry.streamingSize == 0) {
        // Для пустых файлов просто создаем пустой буфер
        imgFile.data.clear();
        imgFile.name = std::string(entry.name);
        imgFile.offset = static_cast<size_t>(offsetBytes);
        imgFile.size = 0;
        imgFile.version = ImgVersion::V2_EXTENDED_SA;
        
        // Извлекаем расширение
        size_t dotPos = imgFile.name.find_last_of('.');
        if (dotPos != std::string::npos) {
            imgFile.extension = imgFile.name.substr(dotPos);
        }
        
        return true;
    }
    
    // Проверяем, не превышает ли размер максимально допустимый для size_t
    if (sizeBytes > SIZE_MAX) {
        //printf("[IMG] Ошибка: Файл '%s' слишком большой (%llu байт > %zu байт)\n", 
        //       entry.name, (unsigned long long)sizeBytes, SIZE_MAX);
        return false;
    }
    
    // Устанавливаем позицию в файле
    file.seekg(static_cast<std::streamoff>(offsetBytes), std::ios::beg);
    if (file.fail()) {
        //printf("[IMG] Ошибка: Не удалось установить позицию %llu для файла '%s'\n", 
        //       (unsigned long long)offsetBytes, entry.name);
        return false;
    }
    
    // Выделяем буфер для данных
    imgFile.data.resize(static_cast<size_t>(sizeBytes));
    
    // Читаем данные
    file.read(reinterpret_cast<char*>(imgFile.data.data()), static_cast<std::streamsize>(sizeBytes));
    if (file.fail() || file.gcount() != static_cast<std::streamsize>(sizeBytes)) {
        //printf("[IMG] Ошибка: Не удалось прочитать %llu байт для файла '%s'\n", 
        //       (unsigned long long)sizeBytes, entry.name);
        imgFile.data.clear();
        return false;
    }
    
    // Заполняем информацию о файле
    imgFile.name = std::string(entry.name);
    imgFile.offset = static_cast<size_t>(offsetBytes);
    imgFile.size = static_cast<size_t>(sizeBytes);
    imgFile.version = ImgVersion::V2_EXTENDED_SA;
    
    // Извлекаем расширение
    size_t dotPos = imgFile.name.find_last_of('.');
    if (dotPos != std::string::npos) {
        imgFile.extension = imgFile.name.substr(dotPos);
    }
    
    //printf("[IMG] Загружен расширенный файл: %s (%llu байт, смещение: %llu)\n", 
    //       entry.name, (unsigned long long)sizeBytes, (unsigned long long)offsetBytes);
    
    return true;
}

// Автоопределение версии IMG файла
img::ImgVersion img::ImgData::detectImgVersion(std::ifstream& file) {
    // Сохраняем текущую позицию
    std::streampos currentPos = file.tellg();
    
    // Читаем первые 4 байта для определения магического числа
    char magic[4];
    file.read(magic, 4);
    
    // Возвращаем позицию
    file.seekg(currentPos);
    
    // Проверяем магическое число
    if (strncmp(magic, "VER2", 4) == 0) {
        // Проверяем, есть ли флаг расширенного формата
        // Для этого читаем заголовок и проверяем дополнительные поля
        file.seekg(8, std::ios::beg); // Пропускаем magic + fileCount
        uint32_t flags;
        file.read(reinterpret_cast<char*>(&flags), 4);
        
        // Возвращаем позицию
        file.seekg(currentPos);
        
        if (flags == 1) {
            return ImgVersion::V2_EXTENDED_SA;
        } else {
            return ImgVersion::V2_GTA_SA;
        }
    } else if (strncmp(magic, "VER3", 4) == 0) {
        return ImgVersion::V3_GTA_IV;
    } else {
        // По умолчанию считаем стандартным форматом
        return ImgVersion::V2_GTA_SA;
    }
}

// Конвертация расширенных записей в стандартные (для совместимости)
img::ImgFileEntry img::ImgData::convertToStandardEntry(const ImgFileEntryExtended& extendedEntry) const {
    ImgFileEntry standardEntry;
    
    // Проверяем, не превышают ли 64-битные значения 32-битные лимиты
    if (extendedEntry.offset > UINT32_MAX) {
        //printf("[IMG] Предупреждение: Смещение %llu превышает 32-битный лимит\n", 
        //       (unsigned long long)extendedEntry.offset);
        standardEntry.offset = UINT32_MAX;
    } else {
        standardEntry.offset = static_cast<uint32_t>(extendedEntry.offset);
    }
    
    if (extendedEntry.streamingSize > UINT16_MAX) {
        //printf("[IMG] Предупреждение: Размер %llu превышает 16-битный лимит\n", 
        //       (unsigned long long)extendedEntry.streamingSize);
        standardEntry.streamingSize = UINT16_MAX;
    } else {
        standardEntry.streamingSize = static_cast<uint16_t>(extendedEntry.streamingSize);
    }
    
    standardEntry.archiveSize = static_cast<uint16_t>(extendedEntry.archiveSize);
    strncpy(standardEntry.name, extendedEntry.name, sizeof(standardEntry.name));
    standardEntry.name[sizeof(standardEntry.name) - 1] = '\0'; // Гарантируем null-termination
    
    return standardEntry;
}

// Получить общий размер всех файлов в байтах
uint64_t img::ImgData::getTotalSizeInBytes() const {
    uint64_t totalSize = 0;
    
    if (isExtended) {
        for (const auto& entry : fileEntriesExtended) {
            totalSize += entry.streamingSize * 2048;
        }
    } else {
        for (const auto& entry : fileEntries) {
            totalSize += entry.streamingSize * 2048;
        }
    }
    
    return totalSize;
}

// Получить размер самого большого файла в байтах
uint64_t img::ImgData::getLargestFileSizeInBytes() const {
    uint64_t largestSize = 0;
    
    if (isExtended) {
        for (const auto& entry : fileEntriesExtended) {
            uint64_t fileSize = entry.streamingSize * 2048;
            if (fileSize > largestSize) {
                largestSize = fileSize;
            }
        }
    } else {
        for (const auto& entry : fileEntries) {
            uint64_t fileSize = entry.streamingSize * 2048;
            if (fileSize > largestSize) {
                largestSize = fileSize;
            }
        }
    }
    
    return largestSize;
}

// Статические утилиты для работы с большими файлами
bool img::isLargeFileSupported(const ImgData& imgData) {
    return imgData.isExtendedFormat();
}

std::string img::getVersionString(ImgVersion version) {
    switch (version) {
        case ImgVersion::V1_GTA3_VC: return "GTA III/VC (IMG v1)";
        case ImgVersion::V2_GTA_SA: return "GTA SA (IMG v2)";
        case ImgVersion::V3_GTA_IV: return "GTA IV (IMG v3)";
        case ImgVersion::V2_EXTENDED_SA: return "GTA SA Extended (FLA)";
        default: return "Unknown";
    }
}

uint64_t img::sectorsToBytes(uint64_t sectors) {
    return sectors * 2048;
}

uint64_t img::bytesToSectors(uint64_t bytes) {
    return (bytes + 2047) / 2048; // Округление вверх
}

// ============================================================================
// DFF PARSER IMPLEMENTATION - ИСПОЛЬЗУЕМ КОД ИЗ DffImporter
// ============================================================================

// Константы для версий GTA
#define GTA_IIIA 0x0003ffff //GTA III?
#define GTA_IIIB 0x0800ffff //GTA III
#define GTA_IIIC 0x00000310 //GTA III
#define GTA_VCA 0x0c02ffff //GTA VC
#define GTA_VCB 0x1003ffff //GTA VC
#define GTA_SA 0x1803ffff //GTA SA

// RW section IDs
namespace RwTypes {
    const uint32_t ATOMIC = 0x14;
    const uint32_t CLUMP_ID = 0x10;
    const uint32_t HANIM_PLG = 0x11e;
    const uint32_t FRAME = 0x253f2fe;
    const uint32_t FRAME_LIST = 0x0E;
    const uint32_t GEOMETRY_LIST = 0x1a;
    const uint32_t EXTENSION = 0x03;
    const uint32_t MATERIAL = 0x07;
    const uint32_t MATERIAL_LIST = 0x08;
    const uint32_t TEXTURE = 0x06;
    const uint32_t GEOMETRY = 0x0F;
    const uint32_t STRUCT = 0x01;
    const uint32_t MATERIAL_EFFECTS_PLG = 0x120;
    const uint32_t REFLECTION_MATERIAL = 0x253f2fc;
    const uint32_t SPECULAR_MATERIAL = 0x253f2f6;
    const uint32_t BIN_MESH_PLG = 0x50e;
    const uint32_t NATIVE_DATA_PLG = 0x0510;
    const uint32_t SKIN_PLG = 0x0116;
    const uint32_t MESH_EXTENSION = 0x0253F2FD;
    const uint32_t NIGHT_VERTEX_COLORS = 0x0253F2F9;
    const uint32_t MORPH_PLG = 0x0105;
    const uint32_t TWO_DFX_PLG = 0x0253F2F8;
    const uint32_t UVANIM_DIC = 0x2B;
    const uint32_t ANIM_ANIMATION = 0x1B;
    const uint32_t UV_ANIM_PLG = 0x135;
    const uint32_t RIGHT_TO_RENDER = 0x1f;
    const uint32_t PIPELINE_SET = 0x0253f2f3;
    const uint32_t STRING = 0x02;
    const uint32_t LIGHT = 0x12;
    const uint32_t CAMERA = 0x13;
}

// Структуры из DffImporter
namespace extensions {
    #pragma pack(1)
    struct GTAHeader {
        uint32_t identifier;
        uint32_t size;
        uint32_t fileVersion;

        bool checkVersion() {
            return (this->fileVersion == GTA_IIIA || this->fileVersion == GTA_IIIB || this->fileVersion == GTA_IIIC
                || this->fileVersion == GTA_VCA || this->fileVersion == GTA_VCB || this->fileVersion == GTA_SA);
        }

        bool operator == (const uint32_t& secID) {
            return (this->identifier == secID && this->checkVersion());
        }

        uint32_t getIdentifier() {
            if (checkVersion())
                return this->identifier;
            else
                return 0;
        }
    };

    struct FrameStructStart {
        GTAHeader structHeader;
        uint32_t frameCount;
    };

    struct ClumpStruct {
        GTAHeader structHeader;
        uint32_t objectCount;
        uint32_t lightCount;
        uint32_t cameraCount;
    };

    struct VertexColors {
        uint8_t red;
        uint8_t blue;
        uint8_t green;
        uint8_t alpha;
    };

    struct FaceBAFC {
        uint16_t bDat;
        uint16_t aDat;
        uint16_t fDat; //material ID
        uint16_t cDat;
    };

    struct UVData {
        float Udat;
        float Vdat;
    };

    struct GeoParams {
        GTAHeader structHeader;
        uint8_t flags;
        uint8_t unused1;
        uint8_t numUVs;
        uint8_t unused2;
        uint32_t faceCount;
        uint32_t vertexCount;
        uint32_t frameCount;
    };

    struct Vector3 {
        float x, y, z;
    };

    struct BinMeshPLGTotal {
        uint32_t isTriStrip;
        uint32_t tSplitCount;
        uint32_t tIndexCount;
    };

    struct BinMeshPLGSub {
        uint32_t indexCount;
        uint32_t materialID;
    };

    struct GTAFrame {
        float transMatrix[4][3];
        unsigned int parent;
        uint32_t unknown;
    };

    struct GeoListStruct {
        GTAHeader structHeader;
        uint32_t gCount;
    };

    struct MaterialStruct {
        GTAHeader structHeader;
        uint32_t unknown1;
        VertexColors matColors;
        uint32_t unknown2;
        uint32_t textureCount;
        float ambient;
        float specular;
        float diffuse;
    };

    struct TextureSectionStruct {
        GTAHeader structHeader;
        uint8_t filterFlags[2];
        uint16_t unknown;
    };
    #pragma pack()
}

// Загрузка DFF файла
bool dff::loadDffFile(const char* filePath, DffData& dffData) {
    return dffData.loadDffFile(filePath);
}

// Загрузка DFF из буфера
bool dff::loadDffFromBuffer(const std::vector<uint8_t>& data, DffData& dffData, const std::string& name) {
    return dffData.loadDffFromBuffer(data, name);
}

// Создание DFF модели
dff::DffModel* dff::createDffModel(const std::vector<uint8_t>& data, const std::string& name) {
    DffData dffData;
    if (dffData.loadDffFromBuffer(data, name)) {
        return new DffModel(dffData.getModel());
    }
    return nullptr;
}

// ============================================================================
// DffData IMPLEMENTATION
// ============================================================================

dff::DffData::DffData() : buffer(nullptr), bufIndex(0), fileSize(0), noIssues(true) {
}

dff::DffData::~DffData() {
    clear();
}

bool dff::DffData::loadDffFile(const char* filePath) {
    printf("[DFF] Загружаем DFF файл: %s\n", filePath);
    
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        printf("[DFF] Ошибка: Не удалось открыть файл: %s\n", filePath);
        return false;
    }
    
    this->fileSize = file.tellg();
    this->buffer = new char[this->fileSize];
    this->bufIndex = 0;
    this->noIssues = true;
    this->fileName = filePath;
    
    file.seekg(0, std::ios::beg);
    file.read(buffer, fileSize);
    file.close();
    
    printf("[DFF] Файл загружен, размер: %zu байт\n", fileSize);
    
    bool result = parseFile();
    if (result) {
        printf("[DFF] Файл успешно распарсен\n");
    } else {
        printf("[DFF] Ошибка парсинга файла\n");
    }
    
    return result;
}

bool dff::DffData::loadDffFromBuffer(const std::vector<uint8_t>& data, const std::string& name) {
    //printf("[DFF] Загружаем DFF из буфера: %s, размер: %zu байт\n", name.c_str(), data.size());
    
    this->fileSize = data.size();
    this->buffer = new char[this->fileSize];
    this->bufIndex = 0;
    this->noIssues = true;
    this->fileName = name;
    
    memcpy(buffer, data.data(), fileSize);
    
    //printf("[DFF] Буфер загружен, размер: %zu байт\n", fileSize);
    
    bool result = parseFile();
    if (result) {
        //printf("[DFF] Буфер успешно распарсен\n");
    } else {
        printf("[loadDffFromBuffer] Ошибка парсинга буфера\n");
    }
    
    return result;
}

template<typename T>
T dff::DffData::readBuffer(const unsigned int& inc) {
    T temp = reinterpret_cast<T>(buffer + bufIndex);
    bufIndex += inc;
    return temp;
}

template<typename T>
T dff::DffData::readNumeric() {
    T output;
    memcpy(&output, buffer + bufIndex, sizeof(T));
    bufIndex += sizeof(T);
    return output;
}

bool dff::DffData::parseFile() {
    //printf("[DFF] Начинаем парсинг файла\n");
    
    extensions::GTAHeader* leHeader = readBuffer<extensions::GTAHeader*>(sizeof(extensions::GTAHeader));
    
    switch(leHeader->getIdentifier()) {
        case RwTypes::UVANIM_DIC:
            //printf("[DFF] Пропускаем UVANIM_DIC\n");
            bufIndex += leHeader->size;
            return parseFile();
            
        case RwTypes::CLUMP_ID:
            //printf("[DFF] Найдена CLUMP секция\n");
            return parseClump();
            
        default:
            printf("[DFF] Неизвестная секция: 0x%08X\n", leHeader->identifier);
        return false;
    }
}

bool dff::DffData::parseClump() {
    //printf("[DFF] Парсим CLUMP\n");
    
    extensions::ClumpStruct* leClump = readBuffer<extensions::ClumpStruct*>(sizeof(extensions::ClumpStruct));
    
    //printf("[DFF] CLUMP: объектов=%u, источников света=%u, камер=%u\n",   leClump->objectCount, leClump->lightCount, leClump->cameraCount);
    
    // Версия GTA III не использует lights и cameras
    if (leClump->structHeader.fileVersion == GTA_IIIA || 
        leClump->structHeader.fileVersion == GTA_IIIB || 
        leClump->structHeader.fileVersion == GTA_IIIC) {
        //printf("[DFF] GTA III версия, корректируем offset\n");
        bufIndex -= 8;
    }
    
    if (!parseFrameList()) {
        printf("[parseClump] Ошибка парсинга FRAME_LIST\n");
        return false;
    }
    
    // Теперь ищем GEOMETRY_LIST, пропуская возможные промежуточные секции
    if (!findAndParseGeometryList()) {
        printf("[parseClump] Ошибка поиска GEOMETRY_LIST\n");
        return false;
    }
    
    //printf("[DFF] CLUMP успешно распарсен\n");
    return true;
}



bool dff::DffData::parseFrameList() {
    //printf("[DFF] Парсим FRAME_LIST\n");
    
    extensions::GTAHeader* theHeader = readBuffer<extensions::GTAHeader*>(sizeof(extensions::GTAHeader));
    if (!(*theHeader == RwTypes::FRAME_LIST)) {
        printf("[parseFrameList] FRAME_LIST не найден\n");
        return false;
    }
    
    extensions::FrameStructStart* frameInfo = readBuffer<extensions::FrameStructStart*>(sizeof(extensions::FrameStructStart));
    //printf("[DFF] Количество фреймов: %u\n", frameInfo->frameCount);
    
    // Пропускаем данные фреймов
    bufIndex += sizeof(extensions::GTAFrame) * frameInfo->frameCount;
    
    //printf("[DFF] FRAME_LIST успешно распарсен\n");
    return true;
}

bool dff::DffData::findAndParseGeometryList() {
    //printf("[DFF] Ищем GEOMETRY_LIST секцию...\n");
    
    // Читаем секции до тех пор, пока не найдем GEOMETRY_LIST
    while (bufIndex < fileSize) {
        if (bufIndex + sizeof(extensions::GTAHeader) > fileSize) {
            printf("[findAndParseGeometryList] Достигнут конец файла, GEOMETRY_LIST не найден\n");
                    return false;
                }
        
        extensions::GTAHeader* sectionHeader = readBuffer<extensions::GTAHeader*>(sizeof(extensions::GTAHeader));
        //printf("[DFF] Найдена секция: 0x%08X, размер: %u, версия: 0x%08X\n",  sectionHeader->identifier, sectionHeader->size, sectionHeader->fileVersion);
        
        switch (sectionHeader->getIdentifier()) {
            case RwTypes::GEOMETRY_LIST:
                //printf("[DFF] Найден GEOMETRY_LIST! Парсим...\n");
                return parseGeometryList();
                
            case RwTypes::EXTENSION:
                //printf("[DFF] Пропускаем EXTENSION секцию\n");
                bufIndex += sectionHeader->size;
                break;
                
            case RwTypes::STRUCT:
                //printf("[DFF] Пропускаем STRUCT секцию\n");
                bufIndex += sectionHeader->size;
                break;
                
            case RwTypes::ATOMIC:
                //printf("[DFF] Пропускаем ATOMIC секцию\n");
                bufIndex += sectionHeader->size;
                break;
                
            case RwTypes::LIGHT:
                //printf("[DFF] Пропускаем LIGHT секцию\n");
                bufIndex += sectionHeader->size;
                break;
                
            case RwTypes::CAMERA:
                //printf("[DFF] Пропускаем CAMERA секцию\n");
                bufIndex += sectionHeader->size;
                break;
                
            default:
                printf("[DFF] Неизвестная секция 0x%08X, пропускаем %u байт\n",   sectionHeader->identifier, sectionHeader->size);
                bufIndex += sectionHeader->size;
                break;
        }
    }
    
    printf("[DFF] GEOMETRY_LIST не найден после перебора всех секций\n");
        return false;
    }
    
bool dff::DffData::parseGeometryList() {
    //printf("[DFF] Парсим GEOMETRY_LIST\n");
    
    // Мы уже прочитали заголовок в findAndParseGeometryList, 
    // поэтому сразу читаем структуру GeoListStruct
    extensions::GeoListStruct* gListStruct = readBuffer<extensions::GeoListStruct*>(sizeof(extensions::GeoListStruct));
    //printf("[DFF] Количество геометрий: %u\n", gListStruct->gCount);
    
    for (uint32_t idx = 0; idx < gListStruct->gCount; idx++) {
        if (!parseGeometry(idx)) {
            printf("[parseGeometryList] Ошибка парсинга геометрии %u\n", idx);
            return false;
        }
    }
    
    //printf("[DFF] GEOMETRY_LIST успешно распарсен\n");
    return true;
}

bool dff::DffData::parseGeometry(const uint32_t& gIndex) {
    //printf("[DFF] Парсим геометрию %u\n", gIndex);
    
    extensions::GTAHeader* theHeader = readBuffer<extensions::GTAHeader*>(sizeof(extensions::GTAHeader));
    if (theHeader->identifier != RwTypes::GEOMETRY) {
        printf("[parseGeometry] Ожидается GEOMETRY, получено: 0x%08X\n", theHeader->identifier);
            return false;
        }
        
    extensions::GeoParams* meshParams = readBuffer<extensions::GeoParams*>(sizeof(extensions::GeoParams));
    //printf("[DFF] Геометрия: флаги=0x%02X, UV=%u, граней=%u, вершин=%u\n",   meshParams->flags, meshParams->numUVs, meshParams->faceCount, meshParams->vertexCount);
    
    // Пропускаем ambient/specular/diffuse для старых версий
    if (meshParams->structHeader.fileVersion == GTA_IIIA || 
        meshParams->structHeader.fileVersion == GTA_IIIB || 
        meshParams->structHeader.fileVersion == GTA_IIIC || 
        meshParams->structHeader.fileVersion == GTA_VCA) {
        bufIndex += 12; // 3 * sizeof(float)
    }
    
    // Читаем цвета вершин если есть
    if (meshParams->flags & 8) { // PRELIT
        //printf("[DFF] Читаем цвета вершин\n");
        bufIndex += meshParams->vertexCount * 4; // sizeof(VertexColors)
    }
    
    // Читаем UV координаты если есть
    if (meshParams->numUVs > 0) {
        //printf("[DFF] Читаем UV координаты\n");
        bufIndex += meshParams->numUVs * meshParams->vertexCount * sizeof(extensions::UVData);
    }
    
    // Читаем информацию о гранях
    //printf("[DFF] Читаем информацию о гранях\n");
    extensions::FaceBAFC* faceInfo = readBuffer<extensions::FaceBAFC*>(sizeof(extensions::FaceBAFC) * meshParams->faceCount);
    
    // Сохраняем полигоны
    model.polygons.clear();
    model.polygons.reserve(meshParams->faceCount);
    
    for (uint32_t i = 0; i < meshParams->faceCount; i++) {
        dff::Polygon polygon;
        polygon.vertex1 = faceInfo[i].aDat;
        polygon.vertex2 = faceInfo[i].bDat;
        polygon.vertex3 = faceInfo[i].cDat;
        polygon.materialId = faceInfo[i].fDat;
        model.polygons.push_back(polygon);
    }
    
    // Пропускаем bounding information (24 байта)
    bufIndex += 24;
    
    // Читаем вершины
    //printf("[DFF] Читаем вершины\n");
    extensions::Vector3* vertexList = readBuffer<extensions::Vector3*>(sizeof(extensions::Vector3) * meshParams->vertexCount);
    
    model.vertices.clear();
    model.vertices.reserve(meshParams->vertexCount);
    
    for (uint32_t i = 0; i < meshParams->vertexCount; i++) {
        dff::Vertex vertex;
        vertex.x = vertexList[i].x;
        vertex.y = vertexList[i].y;
        vertex.z = vertexList[i].z;
        model.vertices.push_back(vertex);
    }
    
    // Читаем нормали если есть
    if (meshParams->flags & 16) { // NORMS
        //printf("[DFF] Читаем нормали\n");
        extensions::Vector3* normalList = readBuffer<extensions::Vector3*>(sizeof(extensions::Vector3) * meshParams->vertexCount);
        
        model.normals.clear();
        model.normals.reserve(meshParams->vertexCount);
        
        for (uint32_t i = 0; i < meshParams->vertexCount; i++) {
            dff::Normal normal;
            normal.x = normalList[i].x;
            normal.y = normalList[i].y;
            normal.z = normalList[i].z;
            model.normals.push_back(normal);
        }
    }
    
    // Парсим список материалов
    if (!parseMaterialList(gIndex)) {
        printf("[parseGeometry] Ошибка парсинга списка материалов\n");
        return false;
    }
    
    //printf("[DFF] Геометрия %u успешно распарсена: %zu вершин, %zu граней\n",  gIndex, model.vertices.size(), model.polygons.size());
    
    return true;
}

bool dff::DffData::parseMaterialList(const uint32_t& geoIndex) {
    //printf("[DFF] Парсим список материалов\n");
    
    extensions::GTAHeader* leHeader = readBuffer<extensions::GTAHeader*>(sizeof(extensions::GTAHeader));
    if (leHeader->getIdentifier() != RwTypes::MATERIAL_LIST) {
        //printf("[parseMaterialList] MATERIAL_LIST не найден\n");
                return false; 
            }
    
    extensions::FrameStructStart* materialListStruct = readBuffer<extensions::FrameStructStart*>(sizeof(extensions::FrameStructStart));
    //printf("[DFF] Количество материалов: %u\n", materialListStruct->frameCount);
    
    // Пропускаем данные материалов пока
    bufIndex += materialListStruct->frameCount * 4;
    
    //printf("[DFF] Список материалов успешно распарсен\n");
    return true;
}

bool dff::DffData::parseMaterial(const uint32_t& matID) {
    // Пока не реализовано
    return true;
}

bool dff::DffData::parseTexture() {
    // Пока не реализовано
    return true;
}

bool dff::DffData::parseBinMesh() {
    // Пока не реализовано
    return true;
}

bool dff::DffData::parseExtension() {
    // Пока не реализовано
    return true;
}

bool dff::DffData::uploadToGPU() {
    return model.loadToGPU();
}

void dff::DffData::clear() {
    fileName.clear();
    model.clear();
    
    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
    bufIndex = 0;
    fileSize = 0;
    noIssues = true;
}

// ============================================================================
// DffModel IMPLEMENTATION
// ============================================================================

bool dff::DffModel::loadToGPU() {
    //printf("[DFF] Загружаем модель в GPU: %zu вершин, %zu полигонов\n", vertices.size(), polygons.size());
    
    // Проверяем, что OpenGL контекст активен
    GLint currentVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    //printf("[loadToGPU] Текущий VAO: %d\n", currentVAO);
    
    // Проверяем, что у нас есть данные для загрузки
    if (vertices.empty()) {
        printf("[loadToGPU] ОШИБКА: Нет вершин для загрузки в GPU\n");
                return false; 
            }
    
    if (polygons.empty()) {
        printf("[loadToGPU] ОШИБКА: Нет полигонов для загрузки в GPU\n");
        return false;
    }
    
    //printf("[loadToGPU] Создаем VAO...\n");
    // Создаем VAO
    glGenVertexArrays(1, &vao);
    if (vao == 0) {
        printf("[loadToGPU] ОШИБКА: Не удалось создать VAO\n");
                        return false; 
                    }
    //printf("[loadToGPU] VAO создан: %u\n", vao);
    
    glBindVertexArray(vao);
    
    //printf("[DFF] Создаем VBO для вершин...\n");
    // Создаем VBO для вершин
    glGenBuffers(1, &vbo);
    if (vbo == 0) {
        printf("[DFF] ОШИБКА: Не удалось создать VBO\n");
        glDeleteVertexArrays(1, &vao);
        vao = 0;
                        return false; 
                    }
    //printf("[DFF] VBO создан: %u\n", vbo);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    size_t vertexDataSize = vertices.size() * sizeof(Vertex);
    //printf("[DFF] Загружаем %zu байт данных вершин в VBO...\n", vertexDataSize);
    glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertices.data(), GL_STATIC_DRAW);
    
    // Проверяем ошибки OpenGL
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("[DFF] ОШИБКА OpenGL при загрузке вершин: 0x%04X\n", error);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        vbo = 0;
        vao = 0;
        return false;
    }
    
    //printf("[DFF] Настраиваем атрибуты вершин...\n");
    // Настраиваем атрибуты вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Добавляем атрибут для нормалей (если они есть)
    if (!normals.empty()) {
        //printf("[DFF] Настраиваем атрибут для нормалей...\n");
        
        // Создаем VBO для нормалей
        glGenBuffers(1, &normalVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(Normal), normals.data(), GL_STATIC_DRAW);
        
        // Настраиваем атрибут нормалей
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Normal), (void*)0);
        glEnableVertexAttribArray(1);
        //printf("[DFF] Атрибут нормалей включен (%zu нормалей)\n", normals.size());
        } else {
        //printf("[DFF] ВНИМАНИЕ: Нормали не найдены, создаем их автоматически...\n");
        // Создаем простые нормали для каждой грани
        std::vector<Normal> faceNormals;
        faceNormals.reserve(polygons.size());
        
        for (const auto& poly : polygons) {
            // Вычисляем нормаль грани
            const Vertex& v1 = vertices[poly.vertex1];
            const Vertex& v2 = vertices[poly.vertex2];
            const Vertex& v3 = vertices[poly.vertex3];
            
            // Вычисляем векторы рёбер
            float edge1x = v2.x - v1.x;
            float edge1y = v2.y - v1.y;
            float edge1z = v2.z - v1.z;
            
            float edge2x = v3.x - v1.x;
            float edge2y = v3.y - v1.y;
            float edge2z = v3.z - v1.z;
            
            // Векторное произведение для получения нормали
            float nx = edge1y * edge2z - edge1z * edge2y;
            float ny = edge1z * edge2x - edge1x * edge2z;
            float nz = edge1x * edge2y - edge1y * edge2x;
            
            // Нормализуем
            float length = sqrtf(nx * nx + ny * ny + nz * nz);
            if (length > 0.0001f) {
                nx /= length;
                ny /= length;
                nz /= length;
            }
            
            faceNormals.push_back(Normal(nx, ny, nz));
        }
        
        // Создаем массив нормалей для вершин
        std::vector<Normal> vertexNormals(vertices.size(), Normal(0.0f, 0.0f, 0.0f));
        
        // Присваиваем нормали граней к вершинам
        for (size_t i = 0; i < polygons.size(); i++) {
            const auto& poly = polygons[i];
            const auto& normal = faceNormals[i];
            
            vertexNormals[poly.vertex1].x += normal.x;
            vertexNormals[poly.vertex1].y += normal.y;
            vertexNormals[poly.vertex1].z += normal.z;
            
            vertexNormals[poly.vertex2].x += normal.x;
            vertexNormals[poly.vertex2].y += normal.y;
            vertexNormals[poly.vertex2].z += normal.z;
            
            vertexNormals[poly.vertex3].x += normal.x;
            vertexNormals[poly.vertex3].y += normal.y;
            vertexNormals[poly.vertex3].z += normal.z;
        }
        
        // Нормализуем нормали вершин
        for (auto& normal : vertexNormals) {
            float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            if (length > 0.0001f) {
                normal.x /= length;
                normal.y /= length;
                normal.z /= length;
            }
        }
        
        // Обновляем массив нормалей
        normals = vertexNormals;
        
        // Создаем VBO для нормалей
        glGenBuffers(1, &normalVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(Normal), normals.data(), GL_STATIC_DRAW);
        
        // Настраиваем атрибут нормалей
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Normal), (void*)0);
        glEnableVertexAttribArray(1);
        
        //printf("[DFF] Созданы и настроены нормали для освещения (%zu нормалей)\n", normals.size());
    }
    
    // Создаем EBO для индексов
    if (!polygons.empty()) {
        //printf("[DFF] Создаем EBO для индексов...\n");
        glGenBuffers(1, &ebo);
        if (ebo == 0) {
            printf("[DFF] ОШИБКА: Не удалось создать EBO\n");
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
            vbo = 0;
            vao = 0;
            return false;
        }
        //printf("[DFF] EBO создан: %u\n", ebo);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        
        // Конвертируем полигоны в индексы
        std::vector<uint32_t> indices;
        indices.reserve(polygons.size() * 3);
        for (const auto& poly : polygons) {
            indices.push_back(poly.vertex1);
            indices.push_back(poly.vertex2);
            indices.push_back(poly.vertex3);
        }
        
        size_t indexDataSize = indices.size() * sizeof(uint32_t);
        //printf("[DFF] Загружаем %zu индексов (%zu байт) в EBO...\n", indices.size(), indexDataSize);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSize, indices.data(), GL_STATIC_DRAW);
        
        // Проверяем ошибки OpenGL
        error = glGetError();
        if (error != GL_NO_ERROR) {
            printf("[DFF] ОШИБКА OpenGL при загрузке индексов: 0x%04X\n", error);
            glDeleteBuffers(1, &ebo);
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
            ebo = 0;
            vbo = 0;
            vao = 0;
            return false;
        }
    }
    
    // Отвязываем VAO
    glBindVertexArray(0);
    
    //printf("[DFF] Модель успешно загружена в GPU! VAO=%u, VBO=%u, EBO=%u\n", vao, vbo, ebo);
    return true;
}

void dff::DffModel::clear() {
    name.clear();
    vertices.clear();
    normals.clear();
    uvCoords.clear();
    vertexColors.clear();
    polygons.clear();
    materials.clear();
    
    if (vao) {
        glDeleteVertexArrays(1, &vao);
    vao = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    vbo = 0;
    }
    if (ebo) {
        glDeleteBuffers(1, &ebo);
    ebo = 0;
    }
    if (normalVBO) {
        glDeleteBuffers(1, &normalVBO);
        normalVBO = 0;
    }
}

// ============================================================================
// TEST DFF CREATION
// ============================================================================

// Создать простой тестовый DFF файл (куб)
bool dff::createTestDffFile(const char* filePath) {
    printf("[DFF] Создаем тестовый DFF файл: %s\n", filePath);
    
    // Создаем простую геометрию куба
    dff::DffModel testModel;
    testModel.name = "test_cube";
    
    // 8 вершин куба
    testModel.vertices = {
        {-1.0f, -1.0f, -1.0f},  // 0: левый нижний задний
        { 1.0f, -1.0f, -1.0f},  // 1: правый нижний задний
        { 1.0f,  1.0f, -1.0f},  // 2: правый верхний задний
        {-1.0f,  1.0f, -1.0f},  // 3: левый верхний задний
        {-1.0f, -1.0f,  1.0f},  // 4: левый нижний передний
        { 1.0f, -1.0f,  1.0f},  // 5: правый нижний передний
        { 1.0f,  1.0f,  1.0f},  // 6: правый верхний передний
        {-1.0f,  1.0f,  1.0f}   // 7: левый верхний передний
    };
    
    // 12 треугольников (2 на каждую грань куба)
    testModel.polygons = {
        // Передняя грань (Z+)
        {4, 5, 6}, {4, 6, 7},
        // Задняя грань (Z-)
        {0, 2, 1}, {0, 3, 2},
        // Левая грань (X-)
        {0, 1, 4}, {1, 5, 4},
        // Правая грань (X+)
        {1, 2, 5}, {2, 6, 5},
        // Верхняя грань (Y+)
        {3, 7, 2}, {2, 7, 6},
        // Нижняя грань (Y-)
        {0, 4, 3}, {3, 4, 7}
    };
    
    // Нормали для каждой грани
    testModel.normals = {
        { 0.0f,  0.0f,  1.0f},  // передняя
        { 0.0f,  0.0f, -1.0f},  // задняя
        {-1.0f,  0.0f,  0.0f},  // левая
        { 1.0f,  0.0f,  0.0f},  // правая
        { 0.0f,  1.0f,  0.0f},  // верхняя
        { 0.0f, -1.0f,  0.0f}   // нижняя
    };
    
    printf("[DFF] Тестовая модель создана: %zu вершин, %zu полигонов, %zu нормалей\n",
           testModel.vertices.size(), testModel.polygons.size(), testModel.normals.size());
    
    // Сохраняем в файл (пока просто выводим информацию)
    printf("[DFF] Тестовая модель готова к использованию!\n");
    
    return true;
}
















