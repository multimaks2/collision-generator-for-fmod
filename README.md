**Важное примечание**: код проекта на текущем этапе не строго структурирован и может содержать хаотичность/несогласованность.

### Назначение
Инструмент для чтения и визуализации 3D‑геометрии из Grand Theft Auto: San Andreas. Загружает сцену из `data/gta.dat`, парсит `IPL/IMG`, извлекает `DFF`/коллизии и отображает их в OpenGL 3.3 Core.

### Возможности
- **Загрузка сцены**: чтение `gta.dat`, сбор данных из `IPL` и `IMG`.
- **DFF‑модели**: парсинг из `IMG` или `unpack/` (если IMG недоступен; требуется ручное извлечение), загрузка в GPU (VAO/VBO/EBO).
- **Фильтрация по радиусу**: рендер только видимых объектов (радиус настраивается; при дампе не учитывается).
- **Fallback**: кубы для отсутствующих моделей; попытка автоисправления из `unpack/`.
- **Сцена‑навигатор**: базовая сетка 3000×3000 игровых метров, скайбокс, оси координат.
- **UI**: ImGui‑оверлей для статистики и настроек.

### Технологии
- OpenGL 3.3 Core, GLFW (окно/ввод), GLEW (загрузчик), ImGui (UI), GLM (математика).
- Система координат: X — влево/вправо, Y — вперёд/назад, Z — вверх/вниз.

### Требования к данным
- `data/gta.dat` и соответствующие `*.img` в `models/` (поддерживается автопоиск).
- Опционально: `unpack/` с `*.dff` для fallback.

### Предварительные требования (Windows)
- Visual Studio 2022 (17.x)
  - Workload: Desktop development with C++
  - MSVC toolset: MSVC v143
  - Windows SDK: Windows 10/11 SDK (10.0.19041+)
  - Рекомендуется C++ CMake tools (опционально)

### Сборка и запуск (Windows)
- Сначала запустить `create-projects.bat` в корне проекта (генерация `FMOD_GEOMETRY.sln`).
- Открыть `FMOD_GEOMETRY.sln` (Visual Studio, x64).
- Собрать `Release`.
- Запустить `Bin/x86_64/Release/Core.exe`.
 - Итог: исполняемый файл `Core.exe`; объеденяется с необходимыми DLL (`opengl32.dll`), при этом GLEW линкуется статически (`glew32s`).

### Управление
- WASD — движение; Space/Ctrl — вверх/вниз.
- Мышь — обзор; колесо — FOV; Shift — быстрее; Alt — медленнее.
- R — центр камеры; Esc — выход.

### Дамп геометрии
- Экспорт в `geometry.moonstudio` из меню. Формат: сигнатура, список моделей, позиция/кватернион, треугольники (позиции вершин).

### Ограничения
- Текстуры пока не загружаются.
- Некоторые проблемные `DFF` отображаются как fallback‑кубы.
- Импорт коллизий (`.col`) не реализован полностью.

### Используемые внешние модули (`vendor/`)
- `vendor/glew-2.2.0` — GLEW (статическая линковка `glew32s`)
- `vendor/glfw-master` — GLFW (собирается как `glfw-api`)
- `vendor/imgui-master` — ImGui (ядро + backends: `imgui_impl_glfw`, `imgui_impl_opengl3`)
- `vendor/glm-master` — GLM (header‑only)


### Репозитории
- **GLEW**: [github.com/nigels-com/glew](https://github.com/nigels-com/glew)
- **GLFW**: [github.com/glfw/glfw](https://github.com/glfw/glfw)
- **Dear ImGui**: [github.com/ocornut/imgui](https://github.com/ocornut/imgui)
- **GLM**: [github.com/g-truc/glm](https://github.com/g-truc/glm)
- **Premake (premake5, использована сборка из mtasa-blue)**: [github.com/multitheftauto/mtasa-blue](https://github.com/multitheftauto/mtasa-blue)

<details>
  <summary><strong>Структура файла geometry.moonstudio</strong></summary>

# Структура файла geometry.moonstudio

Файл `geometry.moonstudio` содержит 3D геометрию сцены в бинарном формате (little-endian), соответствующем реализации `Renderer::DumpGeometryToFile`.

## Заголовок файла
- **Сигнатура**: `"gemometry_ms"` — 12 байт
- **Количество моделей**: `uint32_t`

## Тело цикла модели
Для каждой модели последовательно записываются:

### 1. Имя модели
- `uint32_t nameLength` — длина имени в байтах
- `char[nameLength]` — название модели (без дополнительного завершающего символа строки)

### 2. Позиция модели
- **Позиция и поворот**: `7` `float` значений:
  - `x` - координата X
  - `y` - координата Y  
  - `z` - координата Z
  - `rx` - компонент X `кватерниона`
  - `ry` - компонент Y `кватерниона`
  - `rz` - компонент Z `кватерниона`
  - `rw` - компонент W `кватерниона`

### 3. Треугольники
- `uint32_t triangleCount` - количество записей треугольников
- **Треугольники записаны в таком формате** `float` значения:  
  - **Цикл треугольников**
    - **Вершина 1** `x`, `y`, `z`
    - **Вершина 2** `x`, `y`, `z`
    - **Вершина 3** `x`, `y`, `z`
  - конец итерации (следующая модель)

## Примечания
- В формате нет отдельного списка вершин: записываются только треугольники с позициями
- Точки треугольников берутся из `model.vertices` по индексам из `model.polygons` на момент дампа
 - Границы блоков однозначны: парсер знает, сколько моделей (`modelCount`) читать, а внутри каждой — сколько треугольников (`triangleCount`). Прочитав ровно `triangleCount * 9` `float`, чтение для модели завершается, и следующий байт — это начало следующей модели.

## Пример C++ парсера

```cpp
struct GeometryData {
    std::string name;
    float x, y, z;
    float rx, ry, rz, rw;
    std::vector<FMOD_VECTOR> vertices;   // не используется форматом дампа
    std::vector<FMOD_VECTOR> triangles;  // по 3 вершины на треугольник
};

inline uint32_t readUint32LE(FILE* file) {
    if (!file) return 0;
    uint32_t value;
    if (fread(&value, sizeof(uint32_t), 1, file) != 1) return 0;
    return value;
}

inline float readFloatLE(FILE* file) {
    if (!file) return 0.0f;
    float value;
    if (fread(&value, sizeof(float), 1, file) != 1) return 0.0f;
    return value;
}

inline std::string readString(FILE* file, uint32_t length) {
    if (!file || length == 0 || length > 1000) return "";
    std::string result;
    result.resize(length);
    if (fread(&result[0], 1, length, file) != length) return "";
    return result;
}

std::vector<GeometryData> parseFmodGeometryFile(const char* filePath) {
    std::vector<GeometryData> geometries;
    FILE* file = fopen(filePath, "rb");
    if (!file) return geometries;

    char signature[13];
    if (fread(signature, 1, 12, file) != 12) { fclose(file); return geometries; }
    signature[12] = '\0';
    if (strcmp(signature, "gemometry_ms") != 0) { fclose(file); return geometries; }

    uint32_t modelCount = readUint32LE(file);
    if (modelCount == 0 || modelCount > 10000) { fclose(file); return geometries; }

    for (uint32_t i = 0; i < modelCount; ++i) {
        GeometryData g;

        uint32_t nameLength = readUint32LE(file);
        if (nameLength == 0 || nameLength > 1000) continue;
        g.name = readString(file, nameLength);
        if (g.name.empty()) continue;

        g.x = readFloatLE(file);
        g.y = readFloatLE(file);
        g.z = readFloatLE(file);
        g.rx = readFloatLE(file);
        g.ry = readFloatLE(file);
        g.rz = readFloatLE(file);
        g.rw = readFloatLE(file);

        uint32_t triangleCount = readUint32LE(file);
        if (triangleCount == 0 || triangleCount > 100000) continue;

        g.triangles.reserve(triangleCount * 3);
        for (uint32_t t = 0; t < triangleCount; ++t) {
            if (feof(file)) break;
            FMOD_VECTOR v1, v2, v3;
            v1.x = readFloatLE(file); v1.y = readFloatLE(file); v1.z = readFloatLE(file);
            v2.x = readFloatLE(file); v2.y = readFloatLE(file); v2.z = readFloatLE(file);
            v3.x = readFloatLE(file); v3.y = readFloatLE(file); v3.z = readFloatLE(file);
            if (feof(file)) break;
            g.triangles.push_back(v1);
            g.triangles.push_back(v2);
            g.triangles.push_back(v3);
        }

        geometries.push_back(std::move(g));
    }

    fclose(file);
    return geometries;
}
```

</details>



