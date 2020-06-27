set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_ENABLE_PCH   ON  CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/external/spdlog EXCLUDE_FROM_ALL)
target_set_warnings(spdlog
  DISABLE ALL)
