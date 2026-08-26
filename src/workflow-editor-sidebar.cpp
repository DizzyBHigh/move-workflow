#include "workflow-editor-sidebar.h"
#include "workflow-node.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWidget>
#include <utility>

namespace {
class EditorSidebar final : public QWidget {
public:
    EditorSidebar(QWidget *parent, workflow_editor_sidebar_callbacks callbacks)
        : QWidget(parent), callbacks_(std::move(callbacks))
    {
        setObjectName("workflowEditorSidebar"); setMinimumWidth(230); setMaximumWidth(300);
        setStyleSheet("QWidget#workflowEditorSidebar{background:#111820;border:1px solid #27313c;} QLineEdit{background:#10161d;color:#e6edf3;border:1px solid #2d3946;border-radius:4px;padding:5px 8px;min-height:24px;} QListWidget{background:#10161d;color:#dbe4ec;border:1px solid #27333f;border-radius:4px;outline:0;} QListWidget::item{padding:5px 8px;border-radius:3px;} QListWidget::item:hover{background:#1b2834;} QListWidget::item:selected{background:#173f66;color:#ffffff;} QPushButton{background:#17212b;color:#dbe4ec;border:1px solid #2d3946;border-radius:4px;padding:4px 7px;min-height:26px;} QPushButton:hover{background:#20303d;border-color:#3d5266;} QPushButton:disabled{color:#687583;background:#141b22;} QPushButton#triggerButton{background:#173f66;border-color:#2f79b7;} QPushButton#actionButton{background:#26344a;border-color:#49698f;} QLabel#heading{color:#aab6c3;font-size:11px;font-weight:700;letter-spacing:1px;}");
        auto *root = new QVBoxLayout(this); root->setContentsMargins(10,10,10,10); root->setSpacing(6);
        auto *title = new QLabel("NODES", this); title->setObjectName("heading"); root->addWidget(title);
        auto *addRow = new QHBoxLayout; addRow->setSpacing(4); addTrigger_ = button("+  Add Trigger Node"); addTrigger_->setObjectName("triggerButton"); addAction_ = button("+  Add Action Node"); addAction_->setObjectName("actionButton"); addRow->addWidget(addTrigger_); addRow->addWidget(addAction_); root->addLayout(addRow);
        auto *editRow = new QHBoxLayout; editRow->setSpacing(4); edit_ = button("Edit"); copy_ = button("Copy"); paste_ = button("Paste"); editRow->addWidget(edit_); editRow->addWidget(copy_); editRow->addWidget(paste_); root->addLayout(editRow);
        auto *manageRow = new QHBoxLayout; manageRow->setSpacing(4); duplicate_ = button("Duplicate"); remove_ = button("Delete"); manageRow->addWidget(duplicate_); manageRow->addWidget(remove_); root->addLayout(manageRow);
        auto *workflowTitle = new QLabel("WORKFLOW NODES", this); workflowTitle->setObjectName("heading"); root->addWidget(workflowTitle);
        workflowNodes_ = new QListWidget(this); workflowNodes_->setMaximumHeight(150); root->addWidget(workflowNodes_);
        search_ = new QLineEdit(this); search_->setPlaceholderText("Search node library..."); root->addWidget(search_);
        nodes_ = new QListWidget(this); root->addWidget(nodes_, 1);
        addPaletteItem("⚡  Trigger", "trigger"); addPaletteItem("Action Node", "action");
        connect(search_, &QLineEdit::textChanged, this, [this](const QString &text){ filter(text); });
        connect(workflowNodes_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item){ if(item && callbacks_.select_node) callbacks_.select_node(item->data(Qt::UserRole).toByteArray().constData()); });
        connect(nodes_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item){ if(callbacks_.add_node) callbacks_.add_node(item->data(Qt::UserRole).toByteArray().constData()); });
        connect(addTrigger_, &QPushButton::clicked, this, [this]{ if(callbacks_.add_trigger) callbacks_.add_trigger(); });
        connect(addAction_, &QPushButton::clicked, this, [this]{ if(callbacks_.add_node) callbacks_.add_node("action"); });
        connect(edit_, &QPushButton::clicked, this, [this]{ if(callbacks_.edit_node) callbacks_.edit_node(); }); connect(copy_, &QPushButton::clicked, this, [this]{ if(callbacks_.copy_node) callbacks_.copy_node(); }); connect(paste_, &QPushButton::clicked, this, [this]{ if(callbacks_.paste_node) callbacks_.paste_node(); }); connect(duplicate_, &QPushButton::clicked, this, [this]{ if(callbacks_.duplicate_node) callbacks_.duplicate_node(); }); connect(remove_, &QPushButton::clicked, this, [this]{ if(callbacks_.delete_node) callbacks_.delete_node(); });
    }
    void setSelectionState(bool selected, bool paste){edit_->setEnabled(selected);copy_->setEnabled(selected);duplicate_->setEnabled(selected);remove_->setEnabled(selected);paste_->setEnabled(paste);}
    void setWorkflowNodes(const QList<NodeItem *> &nodes, NodeItem *selected)
    {
        QSignalBlocker blocker(workflowNodes_); workflowNodes_->clear(); int selectedRow = -1;
        for (int i = 0; i < nodes.size(); ++i) { auto *node = nodes.at(i); auto *item = new QListWidgetItem(node->nodeName(), workflowNodes_); item->setData(Qt::UserRole, node->id()); if (node == selected) selectedRow = i; }
        if (selectedRow >= 0) workflowNodes_->setCurrentRow(selectedRow);
    }
private:
    QPushButton *button(const char *text){return new QPushButton(text,this);} void addPaletteItem(const char *text,const char *kind){auto *item=new QListWidgetItem(text,nodes_);item->setData(Qt::UserRole,kind);}
    void filter(const QString &text){for(int i=0;i<nodes_->count();++i)nodes_->item(i)->setHidden(!nodes_->item(i)->text().contains(text,Qt::CaseInsensitive));}
    workflow_editor_sidebar_callbacks callbacks_;QLineEdit *search_=nullptr;QListWidget *nodes_=nullptr,*workflowNodes_=nullptr;QPushButton *addTrigger_=nullptr,*addAction_=nullptr,*edit_=nullptr,*copy_=nullptr,*paste_=nullptr,*duplicate_=nullptr,*remove_=nullptr;
};
}
QWidget *create_workflow_editor_sidebar(QWidget *parent,workflow_editor_sidebar_callbacks callbacks){return new EditorSidebar(parent,std::move(callbacks));}
void workflow_editor_sidebar_set_selection_state(QWidget *sidebar,bool selected,bool paste){if(auto *widget=dynamic_cast<EditorSidebar*>(sidebar))widget->setSelectionState(selected,paste);}
void workflow_editor_sidebar_set_workflow_nodes(QWidget *sidebar,const QList<NodeItem *> &nodes,NodeItem *selected){if(auto *widget=dynamic_cast<EditorSidebar*>(sidebar))widget->setWorkflowNodes(nodes,selected);}
