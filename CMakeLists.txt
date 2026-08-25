cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  src/plugin-main.c
  src/workflow-model.h
  src/workflow-manager.c
  src/workflow-manager.h
  src/workflow-manager-copy.c
  src/workflow-manager-copy.h
  src/workflow-manager-ui.cpp
  src/workflow-manager-ui.h
  src/workflow-workspace.cpp
  src/workflow-workspace.h
  src/workflow-scene.cpp
  src/workflow-scene.h
  src/workflow-node.cpp
  src/workflow-node.h
  src/workflow-node-copy.c
  src/workflow-node-copy.h
  src/workflow-node-duplicate-ui.cpp
  src/workflow-node-duplicate-ui.h
  src/workflow-node-settings.cpp
  src/workflow-node-settings.h
  src/workflow-node-settings-action.cpp
  src/workflow-node-settings-trigger.cpp
  src/workflow-node-settings-trigger-data.cpp
  src/workflow-node-settings-common.h
  src/workflow-node-dialog.h
  src/workflow-clipboard.cpp
  src/workflow-clipboard.h
  src/workflow-persistence.cpp
  src/workflow-persistence.h
  src/workflow-persistence-json.cpp
  src/workflow-persistence-json.h
  src/workflow-undo.cpp
  src/workflow-undo.h
  src/workflow-runtime.c
  src/workflow-runtime.h
  src/workflow-move-runtime.c
  src/workflow-move-runtime.h
  src/workflow-shortcuts.c
  src/workflow-shortcuts.h
  src/workflow-test-data.c
  src/workflow-test-menu.c
  src/workflow-test-menu.h
  src/workflow-hotkeys.c
  src/workflow-hotkeys.h
  src/workflow-debug.h
  src/workflow-debug.cpp
  src/workflow-debug-ui.h
  src/workflow-debug-ui.cpp
  src/workflow-engine.h
  src/workflow-engine.cpp
  src/workflow-engine-state.h
  src/workflow-engine-state.cpp
  src/workflow-engine-node.h
  src/workflow-engine-node.cpp
  src/workflow-engine-delay.h
  src/workflow-engine-delay.cpp
  src/workflow-engine-runner.h
  src/workflow-engine-runner.cpp
  src/workflow-engine-runs.h
  src/workflow-engine-runs.cpp
  src/workflow-engine-trigger.h
  src/workflow-engine-trigger.cpp
  src/workflow-engine-service.h
  src/workflow-engine-service.cpp
  src/workflow-action-runtime.h
  src/workflow-action-runtime.cpp
  src/workflow-execution-tests.cpp
  src/move-workflow-tests.cpp
  src/move-workflow-tests.h
  src/move-workflow-editor.cpp
  src/move-workflow-editor.h
  src/workflow-editor-window.cpp
  src/workflow-editor-window.h
  src/workflow-editor-view.cpp
  src/workflow-editor-view.h
  src/workflow-action-list.cpp
  src/workflow-action-list.h
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
