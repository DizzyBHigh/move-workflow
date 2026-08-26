#include "workflow-editor-toolbar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>

namespace {
class EditorToolbar final : public QWidget {
public:
    EditorToolbar(QWidget *parent, workflow_editor_toolbar_callbacks callbacks)
        : QWidget(parent), callbacks_(std::move(callbacks)) {
        setObjectName("workflowEditorToolbar"); auto *layout = new QHBoxLayout(this); layout->setContentsMargins(8, 6, 8, 6); layout->setSpacing(4);
        add_ = button("+ Add Node"); edit_ = button("Edit"); copy_ = button("Copy"); paste_ = button("Paste"); duplicate_ = button("Duplicate"); remove_ = button("Delete"); test_ = button("Test Workflow");
        zoomOut_ = button("−"); zoomReset_ = button("100%"); zoomIn_ = button("+"); fit_ = button("Fit"); close_ = button("Close");
        layout->addWidget(add_); layout->addWidget(edit_); layout->addWidget(copy_); layout->addWidget(paste_); layout->addWidget(duplicate_); layout->addWidget(remove_); layout->addWidget(test_); layout->addStretch(); layout->addWidget(zoomOut_); layout->addWidget(zoomReset_); layout->addWidget(zoomIn_); layout->addWidget(fit_); layout->addWidget(close_);
        test_->setVisible(static_cast<bool>(callbacks_.test));
        connect(add_, &QPushButton::clicked, this, [this] { if (callbacks_.add) callbacks_.add(); }); connect(edit_, &QPushButton::clicked, this, [this] { if (callbacks_.edit) callbacks_.edit(); });
        connect(copy_, &QPushButton::clicked, this, [this] { if (callbacks_.copy) callbacks_.copy(); }); connect(paste_, &QPushButton::clicked, this, [this] { if (callbacks_.paste) callbacks_.paste(); });
        connect(duplicate_, &QPushButton::clicked, this, [this] { if (callbacks_.duplicate) callbacks_.duplicate(); }); connect(remove_, &QPushButton::clicked, this, [this] { if (callbacks_.remove) callbacks_.remove(); });
        connect(test_, &QPushButton::clicked, this, [this] { if (callbacks_.test) callbacks_.test(); }); connect(zoomOut_, &QPushButton::clicked, this, [this] { if (callbacks_.zoom_out) callbacks_.zoom_out(); });
        connect(zoomReset_, &QPushButton::clicked, this, [this] { if (callbacks_.zoom_reset) callbacks_.zoom_reset(); }); connect(zoomIn_, &QPushButton::clicked, this, [this] { if (callbacks_.zoom_in) callbacks_.zoom_in(); });
        connect(fit_, &QPushButton::clicked, this, [this] { if (callbacks_.fit) callbacks_.fit(); }); connect(close_, &QPushButton::clicked, this, [this] { if (callbacks_.close) callbacks_.close(); });
    }
    void setSelectionState(bool selected, bool paste) { edit_->setEnabled(selected); copy_->setEnabled(selected); duplicate_->setEnabled(selected); remove_->setEnabled(selected); paste_->setEnabled(paste); }
private:
    QPushButton *button(const char *text) { return new QPushButton(text, this); }
    workflow_editor_toolbar_callbacks callbacks_; QPushButton *add_ = nullptr; QPushButton *edit_ = nullptr; QPushButton *copy_ = nullptr; QPushButton *paste_ = nullptr; QPushButton *duplicate_ = nullptr; QPushButton *remove_ = nullptr;
    QPushButton *test_ = nullptr; QPushButton *zoomOut_ = nullptr; QPushButton *zoomReset_ = nullptr; QPushButton *zoomIn_ = nullptr; QPushButton *fit_ = nullptr; QPushButton *close_ = nullptr;
};
}
QWidget *create_workflow_editor_toolbar(QWidget *parent, workflow_editor_toolbar_callbacks callbacks)
{return new EditorToolbar(parent, std::move(callbacks));}
void workflow_editor_toolbar_set_selection_state(QWidget *toolbar, bool selected, bool paste)
{if (auto *widget = dynamic_cast<EditorToolbar *>(toolbar)) widget->setSelectionState(selected, paste);}
