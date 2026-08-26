#include "workflow-editor-toolbar.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QWidget>

namespace {
class EditorToolbar final : public QWidget {
public:
    EditorToolbar(QWidget *parent, workflow_manager_t *manager,
                  workflow_editor_toolbar_callbacks callbacks)
        : QWidget(parent), manager_(manager), callbacks_(std::move(callbacks)) {
        setObjectName("workflowEditorToolbar");
        setStyleSheet("QWidget#workflowEditorToolbar{background:#111820;border:1px solid #27313c;} QLabel{color:#aab6c3;} QComboBox{background:#17212b;color:#e6edf3;border:1px solid #2d3946;border-radius:4px;padding:4px 8px;min-height:27px;} QComboBox:hover{border-color:#3d5266;} QComboBox QAbstractItemView{background:#17212b;color:#e6edf3;selection-background-color:#245b8d;} QPushButton{background:#17212b;color:#dbe4ec;border:1px solid #2d3946;border-radius:4px;padding:4px 8px;min-height:27px;} QPushButton:hover{background:#20303d;border-color:#3d5266;} QPushButton:disabled{color:#687583;background:#141b22;} QCheckBox{color:#dbe4ec;spacing:5px;}");
        auto *layout = new QHBoxLayout(this); layout->setContentsMargins(7,5,7,5); layout->setSpacing(4);
        layout->addWidget(new QLabel("Workflow", this)); workflow_ = new QComboBox(this); workflow_->setMinimumWidth(230); layout->addWidget(workflow_);
        add_ = button("+"); copy_ = button("Copy"); rename_ = button("Rename"); remove_ = button("Delete"); import_ = button("Import"); export_ = button("Export");
        enabled_ = new QCheckBox("Enabled", this); layout->addWidget(add_); layout->addWidget(copy_); layout->addWidget(rename_); layout->addWidget(remove_); layout->addWidget(import_); layout->addWidget(export_); layout->addWidget(enabled_); layout->addStretch();
        zoomOut_ = button("−"); zoomReset_ = button("100%"); zoomIn_ = button("+"); fit_ = button("Fit"); close_ = button("Close");
        layout->addWidget(zoomOut_); layout->addWidget(zoomReset_); layout->addWidget(zoomIn_); layout->addWidget(fit_); layout->addWidget(close_);
        connect(workflow_, &QComboBox::currentIndexChanged, this, [this](int index) { if (index >= 0 && callbacks_.select_workflow) callbacks_.select_workflow(workflow_->itemData(index).toByteArray().constData()); });
        connect(add_, &QPushButton::clicked, this, [this] { createWorkflow(); }); connect(copy_, &QPushButton::clicked, this, [this] { duplicateWorkflow(); }); connect(rename_, &QPushButton::clicked, this, [this] { if (callbacks_.rename_workflow) callbacks_.rename_workflow(); refresh(); }); connect(remove_, &QPushButton::clicked, this, [this] { if (callbacks_.delete_workflow) callbacks_.delete_workflow(); refresh(); });
        connect(import_, &QPushButton::clicked, this, [this] { if (callbacks_.import_workflow) callbacks_.import_workflow(); refresh(); }); connect(export_, &QPushButton::clicked, this, [this] { if (callbacks_.export_workflow) callbacks_.export_workflow(); }); connect(enabled_, &QCheckBox::toggled, this, [this](bool value) { if (callbacks_.set_workflow_enabled) callbacks_.set_workflow_enabled(value); });
        connect(zoomOut_, &QPushButton::clicked, this, [this] { if (callbacks_.zoom_out) callbacks_.zoom_out(); }); connect(zoomReset_, &QPushButton::clicked, this, [this] { if (callbacks_.zoom_reset) callbacks_.zoom_reset(); }); connect(zoomIn_, &QPushButton::clicked, this, [this] { if (callbacks_.zoom_in) callbacks_.zoom_in(); }); connect(fit_, &QPushButton::clicked, this, [this] { if (callbacks_.fit) callbacks_.fit(); }); connect(close_, &QPushButton::clicked, this, [this] { if (callbacks_.close) callbacks_.close(); });
        refresh();
    }
    void refresh() {
        const auto *selected = workflow_manager_selected_const(manager_); const QString id = selected ? QString::fromUtf8(selected->id) : QString();
        workflow_->blockSignals(true); workflow_->clear(); int selectedIndex = -1;
        for (size_t i = 0; i < manager_->workflow_count; ++i) { const auto &w = manager_->workflows[i]; workflow_->addItem(QString::fromUtf8(w.name), QByteArray(w.id)); if (id == QString::fromUtf8(w.id)) selectedIndex = static_cast<int>(i); }
        if (selectedIndex >= 0) workflow_->setCurrentIndex(selectedIndex); workflow_->blockSignals(false); const bool has = selected != nullptr; copy_->setEnabled(has); rename_->setEnabled(has); remove_->setEnabled(has); enabled_->setEnabled(has); enabled_->blockSignals(true); enabled_->setChecked(has && selected->enabled); enabled_->blockSignals(false);
    }
private:
    QPushButton *button(const char *text) { auto *b = new QPushButton(text, this); b->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); return b; }
    void createWorkflow() { bool ok=false; const QString name=QInputDialog::getText(this,"New Workflow","Workflow name:",QLineEdit::Normal,"New Workflow",&ok); if(ok&&!name.trimmed().isEmpty()&&callbacks_.create_workflow) { callbacks_.create_workflow(name.trimmed().toUtf8().constData()); refresh(); } }
    void duplicateWorkflow() { const auto *s=workflow_manager_selected_const(manager_); const QString base=s?QString::fromUtf8(s->name)+" Copy":"Workflow Copy"; bool ok=false; const QString name=QInputDialog::getText(this,"Duplicate Workflow","Workflow name:",QLineEdit::Normal,base,&ok); if(ok&&!name.trimmed().isEmpty()&&callbacks_.duplicate_workflow) { callbacks_.duplicate_workflow(name.trimmed().toUtf8().constData()); refresh(); } }
    workflow_manager_t *manager_; workflow_editor_toolbar_callbacks callbacks_; QComboBox *workflow_=nullptr; QCheckBox *enabled_=nullptr; QPushButton *add_=nullptr,*copy_=nullptr,*rename_=nullptr,*remove_=nullptr,*import_=nullptr,*export_=nullptr,*zoomOut_=nullptr,*zoomReset_=nullptr,*zoomIn_=nullptr,*fit_=nullptr,*close_=nullptr;
};
}
QWidget *create_workflow_editor_toolbar(QWidget *parent, workflow_manager_t *manager, workflow_editor_toolbar_callbacks callbacks){return new EditorToolbar(parent,manager,std::move(callbacks));}
void workflow_editor_toolbar_refresh(QWidget *toolbar){if(auto *widget=dynamic_cast<EditorToolbar*>(toolbar))widget->refresh();}
