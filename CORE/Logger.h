#pragma once

#include <string>
#include <imgui.h>

// Глобальные функции логирования для ImGui
// Эти функции можно вызывать из любого места в коде

// Основная функция логирования
void LogToImGui(const std::string& message, const std::string& category = "General", ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

// Специализированные функции для разных категорий
void LogRender(const std::string& message);
void LogSystem(const std::string& message);
void LogImg(const std::string& message);
void LogIpl(const std::string& message);
void LogCol(const std::string& message);
void LogModels(const std::string& message);
void LogDebug(const std::string& message);

// Функции с цветовым кодированием
void LogInfo(const std::string& message);
void LogWarning(const std::string& message);
void LogError(const std::string& message);
void LogSuccess(const std::string& message);

// Макросы для удобного логирования
#define LOG_RENDER(msg) LogRender(msg)
#define LOG_SYSTEM(msg) LogSystem(msg)
#define LOG_IMG(msg) LogImg(msg)
#define LOG_IPL(msg) LogIpl(msg)
#define LOG_COL(msg) LogCol(msg)
#define LOG_MODELS(msg) LogModels(msg)
#define LOG_DEBUG(msg) LogDebug(msg)

// Макросы для форматированного логирования
#define LOG_IMG_F(format, ...) LogImgF(format, ##__VA_ARGS__)
#define LOG_IPL_F(format, ...) LogIplF(format, ##__VA_ARGS__)
#define LOG_COL_F(format, ...) LogColF(format, ##__VA_ARGS__)

#define LOG_INFO(msg) LogInfo(msg)
#define LOG_WARN(msg) LogWarning(msg)
#define LOG_ERROR(msg) LogError(msg)
#define LOG_SUCCESS(msg) LogSuccess(msg)

// Макрос для логирования с автоматическим определением категории по тегу [Tag]
#define LOG_TAGGED(msg) LogTaggedMessage(msg)
void LogTaggedMessage(const std::string& message);

// Перегрузка для printf-подобного форматирования
void LogColF(const char* format, ...);

// Функция для установки экземпляра Menu (вызывается из Menu::Initialize)
void SetMenuInstance(void* menu, void (*addLogFunc)(void*, const std::string&, const std::string&, ImVec4));
