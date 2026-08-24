  src/plugin-main.c
  src/move-workflow-tests.cpp
  src/move-workflow-tests.h
  src/move-workflow-editor.cpp
  src/move-workflow-editor.h
  src/workflow-editor-window.cpp
  src/workflow-editor-window.h
  src/workflow-editor-view.cpp
  src/workflow-editor-view.h
  src/workflow-node.cpp
  src/workflow-node.h
  src/workflow-action-list.cpp
  src/workflow-action-list.h
  src/workflow-node-settings.cpp
  src/workflow-node-settings.h
  src/workflow-node-settings-common.h
  src/workflow-node-settings-action.cpp
  src/workflow-node-settings-trigger.cpp
  src/workflow-node-settings-trigger-data.cpp
  src/workflow-node-dialog.h
  src/workflow-scene.cpp
  src/workflow-scene.h
  src/move-workflow-editor-panel.cpp
  src/move-workflow-menu-cleanup.cpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})