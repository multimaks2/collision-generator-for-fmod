#include "Logger.h"
#include <iostream>
#include <cstdio>
#include <cstdarg>

// Тип функции для добавления лога
typedef void (*AddLogFunc)(void*, const std::string&, const std::string&, ImVec4);

// Глобальные указатели для доступа к системе логирования
static void* g_menuInstance = nullptr;
static AddLogFunc g_addLogFunc = nullptr;

// Функция для установки экземпляра Menu (вызывается из Menu::Initialize)
void SetMenuInstance(void* menu, AddLogFunc addLogFunc) {
    g_menuInstance = menu;
    g_addLogFunc = addLogFunc;
}

// Основная функция логирования
void LogToImGui(const std::string& message, const std::string& category, ImVec4 color) {
    if (g_menuInstance && g_addLogFunc) {
        // Вызываем функцию через указатель на функцию
        g_addLogFunc(g_menuInstance, message, category, color);
    }
    
    // Также выводим в стандартную консоль для отладки
    printf("[%s] %s\n", category.c_str(), message.c_str());
}

// Специализированные функции для разных категорий
void LogRender(const std::string& message) {
    LogToImGui(message, "Render", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
}

void LogSystem(const std::string& message) {
    LogToImGui(message, "System", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
}

void LogImg(const std::string& message) {
    LogToImGui(message, "IMG", ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
}

void LogIpl(const std::string& message) {
    LogToImGui(message, "IPL", ImVec4(0.8f, 0.0f, 1.0f, 1.0f));
}

void LogCol(const std::string& message) {
    LogToImGui(message, "COL", ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
}

void LogModels(const std::string& message) {
    LogToImGui(message, "Models", ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
}

void LogDebug(const std::string& message) {
    LogToImGui(message, "Debug", ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
}

// Функции с цветовым кодированием
void LogInfo(const std::string& message) {
    LogToImGui(message, "Info", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
}

void LogWarning(const std::string& message) {
    LogToImGui(message, "Warning", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
}

void LogError(const std::string& message) {
    LogToImGui(message, "Error", ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void LogSuccess(const std::string& message) {
    LogToImGui(message, "Success", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
}

// Функция для логирования с автоматическим определением категории по тегу [Tag]
void LogTaggedMessage(const std::string& message) {
    std::string category = "General";
    ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    
    size_t start = message.find('[');
    size_t end = message.find(']');
    
    if (start != std::string::npos && end != std::string::npos && end > start) {
        std::string tag = message.substr(start + 1, end - start - 1);
        
        // Определяем категорию и цвет по тегу
        if (tag == "Console" || tag == "System") {
            category = "System";
            color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
        } else if (tag == "Renderer" || tag == "Render") {
            category = "Render";
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        } else if (tag == "IMG" || tag == "Img") {
            category = "IMG";
            color = ImVec4(0.0f, 0.8f, 1.0f, 1.0f);
        } else if (tag == "IPL" || tag == "Ipl") {
            category = "IPL";
            color = ImVec4(0.8f, 0.0f, 1.0f, 1.0f);
        } else if (tag == "COL" || tag == "Col") {
            category = "COL";
            color = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
        } else if (tag == "DFF" || tag == "Models") {
            category = "Models";
            color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
        } else if (tag == "Menu" || tag == "Debug") {
            category = "Debug";
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        
        // Убираем тег из сообщения для отображения
        std::string cleanMessage = message.substr(end + 1);
        if (!cleanMessage.empty() && cleanMessage[0] == ' ') {
            cleanMessage = cleanMessage.substr(1); // Убираем пробел после тега
        }
        
        LogToImGui(cleanMessage, category, color);
    } else {
        // Если нет тега, выводим как общее сообщение
        LogToImGui(message, category, color);
    }
}

// Перегрузка для C-строк
void LogToImGui(const char* message, const char* category, ImVec4 color) {
    LogToImGui(std::string(message), std::string(category), color);
}

// Перегрузка для printf-подобного форматирования
void LogToImGuiF(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    LogToImGui(buffer);
}

// Специализированные функции с форматированием
void LogRenderF(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    LogRender(buffer);
}

void LogSystemF(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    LogSystem(buffer);
}

void LogImgF(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    LogImg(buffer);
}

void LogIplF(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    LogIpl(buffer);
}

void LogModelsF(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    LogModels(buffer);
}

void LogColF(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    LogCol(buffer);
}

void LogDebugF(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    LogDebug(buffer);
}
