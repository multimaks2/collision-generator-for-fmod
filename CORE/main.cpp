#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <windows.h>

// Включаем наши заголовочные файлы
#include "Renderer.h"
#include "Loader.h"
#include "Menu.h"
#include "Input.h"
#include "Logger.h"
#include "CollisionGtaSaParser.h"

// Константы для настройки
const int MAX_IPL_OBJECTS_TO_CREATE = 1000000;  // Максимальное количество тестовых кубов
const bool ENABLE_DETAILED_LOGGING = true;   // Детальное логирование первых объектов
const int DETAILED_LOGGING_COUNT = 10;       // Сколько объектов логировать детально

// Функция для создания папки, если она не существует
bool createDirectoryIfNotExists(const std::string& path) {
    std::filesystem::path dirPath(path);
    if (!std::filesystem::exists(dirPath)) {
        try {
            return std::filesystem::create_directories(dirPath);
        } catch (const std::exception& e) {
            LogError("Ошибка создания папки " + path + ": " + e.what());
            return false;
        }
    }
    return true;
}

// Функция для извлечения .col файлов из IMG архива
bool extractColFilesFromImg(const img::ImgData& imgData, const std::string& outputDir) {
    LogCol("Извлекаем .col файлы из IMG: " + imgData.getFileName());
    
    // Создаем папку для .col файлов, если её нет
    if (!createDirectoryIfNotExists(outputDir)) {
        LogError("Не удалось создать папку: " + outputDir);
        return false;
    }
    
    // Получаем все файлы с расширением .col
    auto colFiles = imgData.getFilesByExtension(".col");
    LogCol("Найдено .col файлов: " + std::to_string(colFiles.size()));
    
    int extractedCount = 0;
    for (const auto& colFile : colFiles) {
        std::string outputPath = outputDir + "\\" + colFile.name;
        
        try {
            std::ofstream outFile(outputPath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(reinterpret_cast<const char*>(colFile.data.data()), colFile.data.size());
                outFile.close();
                extractedCount++;
                LogSuccess("Извлечен: " + colFile.name + " (" + std::to_string(colFile.data.size()) + " байт)");
            } else {
                LogError("Не удалось создать файл: " + outputPath);
            }
        } catch (const std::exception& e) {
            LogError("Ошибка при извлечении " + colFile.name + ": " + e.what());
        }
    }
    
    LogSuccess("Извлечено .col файлов: " + std::to_string(extractedCount) + " из " + std::to_string(colFiles.size()));
    return extractedCount > 0;
}



// Функция для извлечения всех .col файлов из всех IMG архивов
void extractAllColFiles(const std::vector<img::ImgData>& imgDataVector, const std::string& outputDir) {
    LogCol("========================================");
    LogCol("ИЗВЛЕЧЕНИЕ .COL ФАЙЛОВ:");
    LogCol("========================================");
    
    int totalColFiles = 0;
    int totalExtracted = 0;
    
    for (const auto& imgData : imgDataVector) {
        // Подсчитываем общее количество .col файлов
        auto colFiles = imgData.getFilesByExtension(".col");
        totalColFiles += colFiles.size();
        
        // Извлекаем .col файлы из текущего IMG
        if (extractColFilesFromImg(imgData, outputDir)) {
            totalExtracted += colFiles.size();
        }
    }
    
    LogCol("========================================");
    LogCol("ИТОГИ ИЗВЛЕЧЕНИЯ:");
    LogCol("========================================");
    LogCol("Всего .col файлов найдено: " + std::to_string(totalColFiles));
    LogCol("Всего .col файлов извлечено: " + std::to_string(totalExtracted));
    LogCol("Папка назначения: " + outputDir);
    LogCol("========================================");
}

// Функция для поиска лучшей версии модели с приоритетом LOD
std::string findBestModelVersion(const std::vector<img::ImgData*>& imgArchives, const std::string& baseName) {
    std::string bestModelName = baseName + ".dff";
    
    // В GTA SA LOD объекты имеют приписку "lod" в НАЧАЛЕ названия
    // Например: u_land_001 -> lodand_001
    
    // Сначала ищем LOD версию (приписка "lod" в начале)
    std::string lodName = "lod" + baseName + ".dff";
    
    // Проверяем lod{baseName}.dff
    if (img::modelExists(imgArchives, lodName)) {
        LogModels("Найдена LOD версия модели: " + lodName + " для " + baseName);
        return lodName;
    }
    
    // Если LOD версии нет, возвращаем обычную версию
    if (img::modelExists(imgArchives, bestModelName)) {
        return bestModelName;
    }
    
    return ""; // Модель не найдена
}

// Функция для проверки существования модели в папке unpack с приоритетом LOD
std::string findBestModelInUnpack(const std::string& baseName) {
    std::string basePath = "unpack\\" + baseName + ".dff";
    std::string lodPath = "unpack\\lod" + baseName + ".dff";
    
    // В GTA SA LOD объекты имеют приписку "lod" в НАЧАЛЕ названия
    // Проверяем lod{baseName}.dff
    if (std::filesystem::exists(lodPath)) {
        LogModels("Найдена LOD версия модели в unpack: lod" + baseName + ".dff");
        return lodPath;
    }
    
    // Если LOD версии нет, возвращаем обычную версию
    if (std::filesystem::exists(basePath)) {
        return basePath;
    }
    
    return ""; // Модель не найдена
}

// Структура для группировки объектов по координатам
struct ObjectGroup {
    float x, y, z;
    std::vector<size_t> objectIndices; // Индексы объектов в этой группе
    std::string bestModelName;         // Лучшая модель для этой группы
    bool hasLodVersion;                // Есть ли LOD версия
};

// Функция для группировки объектов по координатам
std::vector<ObjectGroup> groupObjectsByCoordinates(const std::vector<ipl::IplObject>& objects, float threshold = 1.0f) {
    std::vector<ObjectGroup> groups;
    
    for (size_t i = 0; i < objects.size(); i++) {
        const auto& obj = objects[i];
        bool addedToGroup = false;
        
        // Ищем существующую группу для этих координат
        for (auto& group : groups) {
            float dx = abs(group.x - obj.x);
            float dy = abs(group.y - obj.y);
            float dz = abs(group.z - obj.z);
            
            if (dx <= threshold && dy <= threshold && dz <= threshold) {
                // Добавляем объект в существующую группу
                group.objectIndices.push_back(i);
                addedToGroup = true;
                break;
            }
        }
        
        // Если группа не найдена, создаем новую
        if (!addedToGroup) {
            ObjectGroup newGroup;
            newGroup.x = obj.x;
            newGroup.y = obj.y;
            newGroup.z = obj.z;
            newGroup.objectIndices.push_back(i);
            newGroup.hasLodVersion = false;
            groups.push_back(newGroup);
        }
    }
    
    return groups;
}

// Функция для выбора лучшей модели для группы объектов
std::string selectBestModelForGroup(const std::vector<img::ImgData*>& imgArchives, 
                                   const std::vector<ipl::IplObject>& objects,
                                   const ObjectGroup& group) {
    std::string bestModelName = "";
    bool hasLod = false;
    
    // Проверяем все объекты в группе
    for (size_t objIndex : group.objectIndices) {
        const auto& obj = objects[objIndex];
        std::string modelName = findBestModelVersion(imgArchives, obj.name);
        
        if (!modelName.empty()) {
            // Проверяем, является ли это LOD версией (приписка "lod" в начале)
            if (modelName.find("lod") == 0) { // "lod" в самом начале названия
                if (!hasLod || bestModelName.empty()) {
                    bestModelName = modelName;
                    hasLod = true;
                    LogModels("Выбрана LOD версия для группы: " + modelName);
                }
            } else if (!hasLod && bestModelName.empty()) {
                // Берем обычную версию только если LOD не найден
                bestModelName = modelName;
                LogModels("Выбрана обычная версия для группы: " + modelName);
            }
        }
    }
    
    return bestModelName;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Инициализируем систему логирования ImGui
    LogSystem("Приложение FMOD Geometry Viewer запущено");

    // Инициализируем рендер
    Renderer renderer;
    if (!renderer.Initialize(1280, 720, "FMOD Geometry Viewer")) {
        LogError("Ошибка инициализации рендера");
        return -1;
    }
    
    LogSuccess("Рендер успешно инициализирован");

    // ============================================================================
 // ЭТАП 1: ЗАГРУЗКА ДАННЫХ ИЗ DAT ФАЙЛА
 // ============================================================================

    GtaDatData gtaData;
    if (!dat::loadGtaDat("data\\gta.dat", gtaData)) {
        LogSystem("ОШИБКА: Не удалось загрузить gta.dat");
        printf("Ошибка загрузки gta.dat\n");
        return -1;
    }

    // Получаем все записи из DAT файла
    auto iplEntries = gtaData.getEntriesByType(DataType::IPL);
    auto imgEntries = gtaData.getEntriesByType(DataType::IMG);
    
    LogSystem("Найдено IPL файлов: " + std::to_string(iplEntries.size()));
    LogSystem("Найдено IMG файлов: " + std::to_string(imgEntries.size()));

    // Автопоиск .img файлов в папке models
    std::vector<std::string> autoImgFiles = img::findImgFilesInModelsFolder();
    for (const auto& imgFile : autoImgFiles) {
        imgEntries.push_back({ DataType::IMG, imgFile });
    }

    // Логируем найденные .img файлы
    if (!autoImgFiles.empty()) {
        LogImg("Найдены .img файлы в папке models:");
        for (const auto& imgFile : autoImgFiles) {
            LogImg("  - " + imgFile);
        }
    }
    else {
        LogImg("ПРЕДУПРЕЖДЕНИЕ: В папке models не найдено .img файлов");
    }

    // ============================================================================
    // ЭТАП 2: ПАРСИНГ IPL ФАЙЛОВ И СОБИРАНИЕ ОБЪЕКТОВ
    // ============================================================================

    std::vector<ipl::IplObject> allObjects;

    for (const auto& entry : iplEntries) {
        ipl::IplData iplData;
        if (ipl::loadIplFile(entry.path.c_str(), iplData)) {
            const auto& objects = iplData.getAllObjects();
            allObjects.insert(allObjects.end(), objects.begin(), objects.end());
            LogIpl("IPL загружен: " + entry.path + " (объектов: " + std::to_string(objects.size()) + ")");
        }
        else {
            LogIpl("ОШИБКА: Не удалось загрузить IPL файл: " + entry.path);
        }
    }
    
    LogSystem("Всего собрано объектов из IPL: " + std::to_string(allObjects.size()));
    
    // Анализируем координаты объектов для отладки
    if (!allObjects.empty()) {
        float minX = allObjects[0].x, maxX = allObjects[0].x;
        float minY = allObjects[0].y, maxY = allObjects[0].y;
        float minZ = allObjects[0].z, maxZ = allObjects[0].z;
        
        for (const auto& obj : allObjects) {
            minX = std::min(minX, obj.x);
            maxX = std::max(maxX, obj.x);
            minY = std::min(minY, obj.y);
            maxY = std::max(maxY, obj.y);
            minZ = std::min(minZ, obj.z);
            maxZ = std::max(maxZ, obj.z);
        }
        
        LogSystem("Диапазон координат объектов:");
        LogSystem("  X: от " + std::to_string(minX) + " до " + std::to_string(maxX));
        LogSystem("  Y: от " + std::to_string(minY) + " до " + std::to_string(maxY));
        LogSystem("  Z: от " + std::to_string(minZ) + " до " + std::to_string(maxZ));
    }

    // ============================================================================
    // ЭТАП 3: ЗАГРУЗКА IMG АРХИВОВ
    // ============================================================================

    std::vector<img::ImgData*> loadedImgArchives;

    for (const auto& entry : imgEntries) {
        FILE* testFile = fopen(entry.path.c_str(), "rb");
        if (!testFile) {
            LogImg("ПРЕДУПРЕЖДЕНИЕ: Файл не найден: " + entry.path);
            continue;
        }
        fclose(testFile);

        img::ImgData* imgData = new img::ImgData();
        if (img::loadImgFile(entry.path.c_str(), *imgData)) {
            loadedImgArchives.push_back(imgData);
            LogImg("IMG архив загружен: " + entry.path + " (файлов: " + std::to_string(imgData->getFileCount()) + ")");
        }
        else {
            LogImg("ОШИБКА: Не удалось загрузить IMG архив: " + entry.path);
            delete imgData;
        }
    }
    
    LogSystem("Всего загружено IMG архивов: " + std::to_string(loadedImgArchives.size()));

    // Передаем IMG архивы в Renderer для системы fallback
    renderer.SetImgArchives(loadedImgArchives);
    
    // Проверяем наличие DFF файлов в IMG архивах
    int totalDffFiles = 0;
    for (const auto* imgData : loadedImgArchives) {
        auto dffFiles = imgData->getFilesByExtension(".dff");
        totalDffFiles += dffFiles.size();
    }
    LogSystem("Всего DFF файлов в IMG архивах: " + std::to_string(totalDffFiles));

    // ============================================================================
    // ЭТАП 4: ЗАГРУЗКА МОДЕЛЕЙ В СЦЕНУ (С ПРЕДОТВРАЩЕНИЕМ ДУБЛИКАТОВ)
    // ============================================================================

    int successCount = 0;
    int fallbackCount = 0;
    int debugCount = 0;
    int duplicateCount = 0;

    // Группируем объекты по координатам для предотвращения дубликатов
    LogSystem("Группировка объектов по координатам для предотвращения дубликатов...");
    std::vector<ObjectGroup> objectGroups = groupObjectsByCoordinates(allObjects, 1.0f);
    LogSystem("Создано групп объектов: " + std::to_string(objectGroups.size()) + " из " + std::to_string(allObjects.size()) + " объектов");

    // Обрабатываем каждую группу объектов
    for (size_t groupIndex = 0; groupIndex < objectGroups.size(); groupIndex++) {
        const auto& group = objectGroups[groupIndex];
        
        if (debugCount < 10) {
            LogModels("Группа #" + std::to_string(groupIndex) + ": " + std::to_string(group.objectIndices.size()) + 
                     " объектов в позиции (" + std::to_string(group.x) + ", " + std::to_string(group.y) + ", " + std::to_string(group.z) + ")");
            
            // Показываем детали группы для отладки
            if (group.objectIndices.size() > 1) {
                LogModels("  Объекты в группе:");
                for (size_t objIdx : group.objectIndices) {
                    const auto& obj = allObjects[objIdx];
                    LogModels("    - " + obj.name + " (ID: " + std::to_string(obj.modelId) + ")");
                    
                    // Проверяем, есть ли LOD версия для этого объекта
                    std::string lodCheck = "lod" + obj.name + ".dff";
                    if (img::modelExists(loadedImgArchives, lodCheck)) {
                        LogModels("      -> LOD версия найдена: " + lodCheck);
                    }
                }
            }
            debugCount++;
        }

        // Выбираем лучшую модель для этой группы
        std::string bestModelName = selectBestModelForGroup(loadedImgArchives, allObjects, group);
        bool modelFound = false;

        // Сначала ищем в IMG архивах
        if (!bestModelName.empty()) {
            if (debugCount <= 10) {
                LogModels("Найдена лучшая DFF модель для группы: " + bestModelName);
            }
            
            std::vector<uint8_t> modelData = img::getModelData(loadedImgArchives, bestModelName);

            dff::DffData dffData;
            if (dffData.loadDffFromBuffer(modelData, bestModelName)) {
                const dff::DffModel& model = dffData.getModel();
                // Используем координаты группы (первого объекта)
                const auto& firstObj = allObjects[group.objectIndices[0]];
                renderer.AddDffModel(model, firstObj.name.c_str(), group.x, group.y, group.z, 
                                   firstObj.rx, firstObj.ry, firstObj.rz, firstObj.rw);
                modelFound = true;
                successCount++;
                
                if (debugCount <= 10) {
                    LogSuccess("DFF модель загружена для группы: " + firstObj.name + " (версия: " + bestModelName + ")");
                }
            }
            else {
                LogModels("ОШИБКА: Не удалось загрузить DFF модель: " + bestModelName);
            }
        } else if (debugCount <= 10) {
            LogModels("DFF модель не найдена в IMG для группы в позиции (" + 
                     std::to_string(group.x) + ", " + std::to_string(group.y) + ", " + std::to_string(group.z) + ")");
        }

        // Fallback: ищем в папке unpack
        if (!modelFound) {
            const auto& firstObj = allObjects[group.objectIndices[0]];
            std::string unpackPath = findBestModelInUnpack(firstObj.name);
            
            if (!unpackPath.empty()) {
                dff::DffData dffData;

                if (dffData.loadDffFile(unpackPath.c_str())) {
                    const dff::DffModel& model = dffData.getModel();
                    renderer.AddDffModel(model, firstObj.name.c_str(), group.x, group.y, group.z, 
                                       firstObj.rx, firstObj.ry, firstObj.rz, firstObj.rw);
                    modelFound = true;
                    successCount++;
                    
                    if (debugCount <= 10) {
                        LogSuccess("DFF модель загружена из unpack для группы: " + firstObj.name + " (путь: " + unpackPath + ")");
                    }
                }
            }
        }

        // Если модель не найдена, используем куб как fallback
        if (!modelFound) {
            fallbackCount++;
            const auto& firstObj = allObjects[group.objectIndices[0]];
            renderer.AddTestObject(groupIndex + 1, firstObj.modelId, firstObj.name.c_str(), 
                                 group.x, group.y, group.z, firstObj.rx, firstObj.ry, firstObj.rz, firstObj.rw);
            
            if (debugCount <= 10) {
                LogWarning("Создан fallback куб для группы в позиции (" + 
                          std::to_string(group.x) + ", " + std::to_string(group.y) + ", " + std::to_string(group.z) + ")");
            }
        }

        // Подсчитываем дубликаты
        if (group.objectIndices.size() > 1) {
            duplicateCount += group.objectIndices.size() - 1;
        }
    }

    // ============================================================================
    // ЭТАП 5: ФИНАЛЬНАЯ СТАТИСТИКА
    // ============================================================================
    
    // Отладочная информация о позиции камеры
    LogSystem("========================================");
    LogSystem("ОТЛАДОЧНАЯ ИНФОРМАЦИЯ:");
    LogSystem("========================================");
    LogSystem("Камера находится в позиции (0, -15, 10)");
    LogSystem("Рендер-радиус: 1500");
    LogSystem("Если объекты не видны, проверьте:");
    LogSystem("1) Координаты объектов находятся в пределах рендер-радиуса");
    LogSystem("2) Объекты не находятся за камерой");
    LogSystem("3) Объекты не слишком маленькие для отображения");
    LogSystem("========================================");

    LogSystem("========================================");
    LogSystem("СТАТИСТИКА ЗАГРУЗКИ МОДЕЛЕЙ:");
    LogSystem("========================================");
    LogSystem("Всего объектов в IPL: " + std::to_string(allObjects.size()));
    LogSystem("Создано групп объектов: " + std::to_string(objectGroups.size()));
    LogSystem("Предотвращено дубликатов: " + std::to_string(duplicateCount));
    LogSystem("Загружено DFF моделей: " + std::to_string(successCount));
    LogSystem("Создано fallback кубов: " + std::to_string(fallbackCount));
    LogSystem("========================================");



    // Проверяем, есть ли объекты в сцене
    if (successCount == 0 && fallbackCount == 0) {
        LogError("КРИТИЧЕСКАЯ ОШИБКА: В сцену не загружено ни одной модели!");
        LogError("Проверьте: 1) IPL файлы загружаются корректно 2) DFF модели существуют в IMG архивах");
    }

    // Очищаем память
    renderer.ClearImgArchives(); // Очищаем ссылки в Renderer
    for (auto* imgData : loadedImgArchives) {
        delete imgData;
    }
    loadedImgArchives.clear();



    while (!renderer.ShouldClose()) {
        renderer.BeginFrame();
        renderer.Render();
        renderer.EndFrame();
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    renderer.Shutdown();
    return 0;
}
