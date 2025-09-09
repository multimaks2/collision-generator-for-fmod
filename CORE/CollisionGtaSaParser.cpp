#include "CollisionGtaSaParser.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdarg>
#include <filesystem> // Для работы с файловой системой

// ============================================================================
// Реализация функций класса col
// ============================================================================

// Функция для поиска COL3 секций в файле (как в скрипте 3ds Max)
int col::findCol3Sections(const char* filePath) {
    if (!filePath) {
        LogCol("[COL] Ошибка: Путь к файлу пуст");
        return 0;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LogCol("[COL] Ошибка: Не удалось открыть файл: %s", filePath);
        return 0;
    }

    LogCol("[COL] Анализируем файл: %s", filePath);

    int col3Count = 0;
    uint32_t magic;
    
    // Сканируем файл с конца, как в скрипте 3ds Max
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    
    // Ищем COL3 секции, начиная с конца файла
    for (std::streamsize pos = fileSize - 4; pos >= 0; --pos) {
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&magic), 4);
        
        if (magic == COL3_MAGIC) {
            col3Count++;
            LogCol("[COL] Найдена COL3 секция #%d на позиции: 0x%08X", col3Count, (uint32_t)pos);
            
            // Читаем имя модели для этой секции
            file.seekg(pos + 4); // Пропускаем FourCC
            uint32_t sectionSize;
            file.read(reinterpret_cast<char*>(&sectionSize), 4);
            
            char nameBuffer[23];
            file.read(nameBuffer, 22);
            nameBuffer[22] = '\0';
            
            // Убираем лишние пробелы
            std::string modelName = std::string(nameBuffer);
            while (!modelName.empty() && modelName.back() == ' ') {
                modelName.pop_back();
            }
            
            LogCol("[COL]   Имя модели: %s", modelName.c_str());
            LogCol("[COL]   Размер секции: %u байт", sectionSize);
        }
    }

    file.close();
    
    if (col3Count == 0) {
        LogCol("[COL] COL3 секции не найдены");
    } else {
        LogCol("[COL] Всего найдено COL3 секций: %d", col3Count);
    }
    
    return col3Count;
}

// Основная функция загрузки COL файла
bool col::loadColFile(const char* filePath) {
    if (!filePath) {
        LogCol("[COL] Ошибка: Путь к файлу пуст");
        return false;
    }

    LogCol("[COL] ========================================");
    LogCol("[COL] Начинаем загрузку COL файла: %s", filePath);
    LogCol("[COL] ========================================");

    // Сначала находим все COL3 секции
    int col3Count = findCol3Sections(filePath);
    
    if (col3Count == 0) {
        LogCol("[COL] Ошибка: COL3 секции не найдены в файле");
        return false;
    }

    LogCol("[COL] Успешно найдено %d COL3 секций", col3Count);
    LogCol("[COL] ========================================");
    
    return true;
}

// Функция для логирования в ImGui (пока используем printf)
void col::LogCol(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Пока используем printf, позже можно заменить на ImGui::Text
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

// ============================================================================
// Старые функции (оставляем для совместимости)
// ============================================================================

// Простая функция загрузки COL файла
bool loadColFile(const char* filePath, std::vector<CollisionModel>& models) {
    if (!filePath) {
        printf("[COL] Ошибка: Путь к файлу пуст\n");
        return false;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        printf("[COL] Ошибка: Не удалось открыть файл: %s\n", filePath);
        return false;
    }

    printf("[COL] Загружаем COL файл: %s\n", filePath);

    // Проверяем FourCC
    char fourCC[4];
    file.read(fourCC, 4);

    if (strncmp(fourCC, "COL3", 4) != 0) {
        printf("[COL] Ошибка: Неверный FourCC, ожидается COL3, получено: %.4s\n", fourCC);
        file.close();
        return false;
    }

    // Читаем размер файла
    uint32_t fileSize;
    file.read(reinterpret_cast<char*>(&fileSize), 4);

    // Читаем имя модели (22 символа)
    char nameBuffer[23];
    file.read(nameBuffer, 22);
    nameBuffer[22] = '\0';

    // Убираем лишние пробелы в конце имени
    std::string modelName = std::string(nameBuffer);
    while (!modelName.empty() && modelName.back() == ' ') {
        modelName.pop_back();
    }

    // Читаем ID модели
    int16_t modelId;
    file.read(reinterpret_cast<char*>(&modelId), 2);

    printf("[COL] Загружена модель: %s (ID: %d)\n", modelName.c_str(), modelId);

    // Создаем простую модель
    CollisionModel model;
    model.name = modelName;
    model.modelId = modelId;

    // Пока просто добавляем пустую модель
    models.push_back(model);

    file.close();
    return true;
}

// ============================================================================
// НОВЫЕ ФУНКЦИИ ДЛЯ РАСПАКОВКИ .COL ФАЙЛОВ
// ============================================================================

// Функция для определения типа .col файла по количеству сигнатур COL3
col::ColFileType col::detectColFileType(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return ColFileType::UNKNOWN;
    }
    
    // Читаем весь файл в буфер для поиска сигнатур
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();
    
    // Ищем все вхождения "COL3"
    int col3Count = 0;
    for (size_t i = 0; i <= fileSize - 4; i++) {
        if (buffer[i] == 'C' && buffer[i+1] == 'O' && buffer[i+2] == 'L' && buffer[i+3] == '3') {
            col3Count++;
        }
    }
    
    LogCol("[COL] Файл %s: найдено %d сигнатур COL3", 
           std::filesystem::path(filePath).filename().string().c_str(), col3Count);
    
    if (col3Count == 0) {
        // Проверяем другие форматы
        if (fileSize >= 4) {
            std::string fourCC(buffer.data(), 4);
            if (fourCC == "COLL" || fourCC == "COL2" || fourCC == "COL4") {
                return ColFileType::SINGLE_COLLISION;
            }
        }
        return ColFileType::UNKNOWN;
    } else if (col3Count == 1) {
        return ColFileType::SINGLE_COLLISION; // Волк-одиночка
    } else {
        return ColFileType::COL3_CONTAINER;   // Контейнер
    }
}

// Функция для получения информации о .col файле
bool col::getColFileInfo(const std::string& filePath, ColFileInfo& info) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    info.filePath = filePath;
    
    // Читаем FourCC
    char fourCC[4];
    file.read(fourCC, 4);
    info.fourCC = std::string(fourCC, 4);
    
    // Определяем тип файла
    info.type = detectColFileType(filePath);
    
    // Получаем размер файла
    file.seekg(0, std::ios::end);
    info.fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Если это одиночный collision model, читаем дополнительную информацию
    if (info.type == ColFileType::SINGLE_COLLISION) {
        file.seekg(4); // Пропускаем FourCC
        
        // Читаем размер (для COL2/COL3/COL4)
        if (info.fourCC != "COLL") {
            uint32_t fileSize;
            file.read(reinterpret_cast<char*>(&fileSize), 4);
        }
        
        // Читаем имя модели (22 символа)
        char nameBuffer[23];
        file.read(nameBuffer, 22);
        nameBuffer[22] = '\0';
        
        // Убираем лишние пробелы
        info.modelName = std::string(nameBuffer);
        while (!info.modelName.empty() && info.modelName.back() == ' ') {
            info.modelName.pop_back();
        }
        
        // Читаем ID модели
        file.read(reinterpret_cast<char*>(&info.modelId), 2);
    }
    
    file.close();
    return true;
}

// Функция для создания папки, если её нет
bool col::createDirectoryIfNotExists(const std::string& path) {
    std::filesystem::path dirPath(path);
    if (!std::filesystem::exists(dirPath)) {
        try {
            return std::filesystem::create_directories(dirPath);
        } catch (const std::exception& e) {
            LogCol("[COL] Ошибка создания папки %s: %s", path.c_str(), e.what());
            return false;
        }
    }
    return true;
}

// Функция для поиска всех .col файлов в папке
std::vector<std::string> col::findColFilesInDirectory(const std::string& directory) {
    std::vector<std::string> colFiles;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                if (extension == ".col") {
                    colFiles.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        LogCol("[COL] Ошибка поиска .col файлов в %s: %s", directory.c_str(), e.what());
    }
    
    return colFiles;
}

// Функция для копирования одиночного .col файла
bool col::copySingleColFile(const std::string& filePath, const std::string& outputDir) {
    std::string fileName = std::filesystem::path(filePath).filename().string();
    std::string outputPath = outputDir + "\\" + fileName;
    
    try {
        std::filesystem::copy_file(filePath, outputPath, std::filesystem::copy_options::overwrite_existing);
        LogCol("[COL] Скопирован одиночный .col файл: %s", fileName.c_str());
        return true;
    } catch (const std::exception& e) {
        LogCol("[COL] Ошибка копирования %s: %s", fileName.c_str(), e.what());
        return false;
    }
}

// Функция для извлечения COL3 контейнера
bool col::extractCol3Container(const std::string& filePath, const std::string& outputDir) {
    LogCol("[COL] Начинаем извлечение из COL3 контейнера: %s", 
           std::filesystem::path(filePath).filename().string().c_str());
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LogCol("[COL] Ошибка открытия файла для извлечения");
        return false;
    }
    
    // Читаем весь файл в буфер
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();
    
    // Ищем все позиции сигнатур "COL3"
    std::vector<size_t> col3Positions;
    for (size_t i = 0; i <= fileSize - 4; i++) {
        if (buffer[i] == 'C' && buffer[i+1] == 'O' && buffer[i+2] == 'L' && buffer[i+3] == '3') {
            col3Positions.push_back(i);
        }
    }
    
    LogCol("[COL] Найдено %zu COL3 секций для извлечения", col3Positions.size());
    
    if (col3Positions.size() <= 1) {
        LogCol("[COL] Недостаточно COL3 секций для извлечения");
        return false;
    }
    
    // Извлекаем каждую COL3 секцию как отдельный .col файл
    int extractedCount = 0;
    for (size_t i = 0; i < col3Positions.size(); i++) {
        size_t startPos = col3Positions[i];
        size_t endPos = (i + 1 < col3Positions.size()) ? col3Positions[i + 1] : fileSize;
        size_t sectionSize = endPos - startPos;
        
        // Читаем размер секции (4 байта после "COL3")
        if (startPos + 8 <= fileSize) {
            uint32_t sectionSizeFromHeader;
            memcpy(&sectionSizeFromHeader, &buffer[startPos + 4], 4);
            
            // Размер в заголовке + 8 байт (FourCC + размер)
            size_t actualSize = sectionSizeFromHeader + 8;
            if (startPos + actualSize <= fileSize) {
                endPos = startPos + actualSize;
                sectionSize = actualSize;
            }
        }
        
        // Читаем реальное имя модели из заголовка секции
        std::string sectionName;
        if (startPos + 30 <= fileSize) { // FourCC(4) + Size(4) + Name(22) = 30 байт
            char nameBuffer[23];
            memcpy(nameBuffer, &buffer[startPos + 8], 22); // 8 = FourCC(4) + Size(4)
            nameBuffer[22] = '\0';
            
            // Убираем лишние пробелы в конце имени
            sectionName = std::string(nameBuffer);
            while (!sectionName.empty() && sectionName.back() == ' ') {
                sectionName.pop_back();
            }
            
            // Если имя пустое или содержит недопустимые символы, используем fallback
            if (sectionName.empty() || sectionName.find_first_of("<>:\"/\\|?*") != std::string::npos) {
                std::string baseName = std::filesystem::path(filePath).stem().string();
                sectionName = baseName + "_section_" + std::to_string(i + 1);
            }
        } else {
            // Fallback если не можем прочитать имя
            std::string baseName = std::filesystem::path(filePath).stem().string();
            sectionName = baseName + "_section_" + std::to_string(i + 1);
        }
        
        // Добавляем расширение .col
        sectionName += ".col";
        std::string outputPath = outputDir + "\\" + sectionName;
        
        // Записываем секцию в отдельный файл
        try {
            std::ofstream outFile(outputPath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(&buffer[startPos], sectionSize);
                outFile.close();
                extractedCount++;
                LogCol("[COL] Извлечена секция %zu: %s (%zu байт)", 
                       i + 1, sectionName.c_str(), sectionSize);
            } else {
                LogCol("[COL] Ошибка создания файла: %s", sectionName.c_str());
            }
        } catch (const std::exception& e) {
            LogCol("[COL] Ошибка при извлечении секции %zu: %s", i + 1, e.what());
        }
    }
    
    LogCol("[COL] Успешно извлечено %d секций из контейнера", extractedCount);
    return extractedCount > 0;
}

// Функция для распаковки .col файла в папку unpack_col
bool col::unpackColFile(const std::string& filePath, const std::string& outputDir) {
    ColFileInfo info;
    if (!getColFileInfo(filePath, info)) {
        LogCol("[COL] Ошибка получения информации о файле: %s", filePath.c_str());
        return false;
    }
    
    LogCol("[COL] Обрабатываем .col файл: %s", std::filesystem::path(filePath).filename().string().c_str());
    LogCol("[COL] Тип: %s, FourCC: %s, Размер: %zu байт", 
           info.type == ColFileType::COL3_CONTAINER ? "COL3 контейнер" : 
           info.type == ColFileType::SINGLE_COLLISION ? "Одиночный collision model" : "Неизвестный",
           info.fourCC.c_str(), info.fileSize);
    
    if (info.type == ColFileType::COL3_CONTAINER) {
        return extractCol3Container(filePath, outputDir);
    } else if (info.type == ColFileType::SINGLE_COLLISION) {
        return copySingleColFile(filePath, outputDir);
    } else {
        LogCol("[COL] Неизвестный тип файла: %s", filePath.c_str());
        return false;
    }
}

// Функция для распаковки всех .col файлов из папки col в unpack_col
bool col::unpackAllColFiles(const std::string& colDir, const std::string& unpackDir) {
    LogCol("[COL] ========================================");
    LogCol("[COL] НАЧИНАЕМ РАСПАКОВКУ .COL ФАЙЛОВ");
    LogCol("[COL] ========================================");
    LogCol("[COL] Папка источника: %s", colDir.c_str());
    LogCol("[COL] Папка назначения: %s", unpackDir.c_str());
    
    // Создаем папку назначения, если её нет
    if (!createDirectoryIfNotExists(unpackDir)) {
        LogCol("[COL] Ошибка создания папки назначения");
        return false;
    }
    
    // Ищем все .col файлы в папке col
    auto colFiles = findColFilesInDirectory(colDir);
    LogCol("[COL] Найдено .col файлов: %zu", colFiles.size());
    
    if (colFiles.empty()) {
        LogCol("[COL] .col файлы не найдены в папке %s", colDir.c_str());
        return false;
    }
    
    // Обрабатываем каждый .col файл
    int successCount = 0;
    int errorCount = 0;
    
    for (const auto& colFile : colFiles) {
        if (unpackColFile(colFile, unpackDir)) {
            successCount++;
        } else {
            errorCount++;
        }
    }
    
    LogCol("[COL] ========================================");
    LogCol("[COL] ИТОГИ РАСПАКОВКИ:");
    LogCol("[COL] ========================================");
    LogCol("[COL] Всего файлов: %zu", colFiles.size());
    LogCol("[COL] Успешно обработано: %d", successCount);
    LogCol("[COL] Ошибок: %d", errorCount);
    LogCol("[COL] ========================================");
    
    return successCount > 0;
}
