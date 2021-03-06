project(gui)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

set(IMGUI_VERSION "1.82")

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")

find_package(ImGui ${IMGUI_VERSION} REQUIRED)

ucm_add_files(
  "${IMGUI_SOURCES}"
  "${IMGUI_DEMO_SOURCES}"
  # "${IMGUI_INCLUDE_DIR}/imgui.cpp"
  # "${IMGUI_INCLUDE_DIR}/imgui_draw.cpp"
  # "${IMGUI_INCLUDE_DIR}/imgui_demo.cpp"
  "${IMGUI_INCLUDE_DIR}/backends/imgui_impl_glfw.cpp"
  "util.cpp"
  "manager.cpp"
  "window.cpp"
  "fault-window.cpp"
  "movement-window.cpp"
  "manual-movement-window.cpp"
  "metadata-window.cpp"
  "status-window.cpp"
  "liquid-status-window.cpp"
  "liquid-control-window.cpp"
  "plc-trigger-window.cpp"
  "speed-profile-window.cpp"
  "system-info-window.cpp"
  TO
  SOURCES
)

if(OPENGL3)
  message("-- Using OpenGL3")
  ucm_add_files("${IMGUI_INCLUDE_DIR}/backends/imgui_impl_opengl3.cpp" TO SOURCES)
elseif(OPENGL2)
  message("-- Using OpenGL2")
  ucm_add_files("${IMGUI_INCLUDE_DIR}/backends/imgui_impl_opengl2.cpp" TO SOURCES)
endif()

ucm_add_target(
  NAME
  gui
  TYPE
  STATIC
  SOURCES
  ${SOURCES}
  UNITY
  CPP_PER_UNITY
  20
  PCH_FILE
  "gui.hpp"
)

add_library("${PROJECT_NAMESPACE}::gui" ALIAS gui)

target_include_directories(
  gui
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
         "$<BUILD_INTERFACE:${OPENGL_INCLUDE_DIR}>"
         "$<BUILD_INTERFACE:${OPENGL_EGL_INCLUDE_DIRS}>"
         "$<BUILD_INTERFACE:${IMGUI_INCLUDE_DIR}>"
         "$<BUILD_INTERFACE:${GLAD_INCLUDE_DIRS}>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_compile_definitions(gui PUBLIC GLFW_INCLUDE_NONE)

target_compile_definitions(gui PUBLIC GLFW_INCLUDE_NONE)

if(OPENGL3)
  target_compile_definitions(gui PUBLIC OPENGL3_EXIST IMGUI_IMPL_OPENGL_LOADER_GLAD)
elseif(OPENGL2)
  target_compile_definitions(gui PUBLIC OPENGL2_EXIST)
endif()

target_link_libraries(
  gui
  PUBLIC ${CMAKE_THREAD_LIBS_INIT}
         ${CMAKE_DL_LIBS}
         spdlog::spdlog
         glfw
         ${OPENGL_LIBRARIES}
         ${GLAD_LIBRARIES}
         "${PROJECT_NAMESPACE}::util"
         "${PROJECT_NAMESPACE}::core"
         "${PROJECT_NAMESPACE}::algo"
         "${PROJECT_NAMESPACE}::device"
         "${PROJECT_NAMESPACE}::mechanism"
         "${PROJECT_NAMESPACE}::machine"
)

target_set_warnings(gui ENABLE ALL DISABLE Annoying)

set_target_properties(
  gui
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED YES
             CXX_EXTENSIONS NO
)

target_compile_options(gui PRIVATE -Wno-extra-semi-stmt -Wno-alloca -Wno-cast-qual)

target_enable_lto(gui optimized)
