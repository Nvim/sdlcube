#pragma once

#include "spdlog/logger.h"
#include <memory>

struct Logger
{
public:
  static void Init();
  static spdlog::logger& Get();

private:
  static inline std::shared_ptr<spdlog::logger> logger_;
};

#define LOG_TRACE(...) Logger::Get().trace(__VA_ARGS__);
#define LOG_DEBUG(...) Logger::Get().debug(__VA_ARGS__);
#define LOG_INFO(...) Logger::Get().info(__VA_ARGS__);
#define LOG_WARN(...) Logger::Get().warn(__VA_ARGS__);
#define LOG_ERROR(...) Logger::Get().error(__VA_ARGS__);
#define LOG_CRITICAL(...) Logger::Get().critical(__VA_ARGS__);
