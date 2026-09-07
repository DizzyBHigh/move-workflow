#pragma once

#include <functional>

class QWidget;

void workflow_editor_sidebar_install_play_buttons(
    QWidget *sidebar, std::function<const char *()> workflow_id_provider);
