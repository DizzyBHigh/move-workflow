#include "workflow-editor-sidebar.h"
#include "workflow-editor-sidebar-icons.h"
#include "workflow-editor-node-order.hpp"
#include "workflow-node.h"

#include <QDropEvent>
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
bool node_configured(const workflow_node_t *node)
{
    if (!node || node->type == WORKFLOW_NODE_TRIGGER) return true;
    if (node->action.kind == WORKFLOW_CHANGE_SCENE) return node->action.scene_name[0] != '\0';
    return node->action.scene_name[0] != '\0' && node->action.filter_name[0] != '\0' && node->action.filter_id[0] != '\0';
}

class ReorderList final : public QListWidget {
public:
    using Reordered = std::function<void()>;
    explicit ReorderList(QWidget *parent) : QListWidget(parent) {}
    void setReorderedCallback(Reordered callback) { reordered_ = std::move(callback); }
protected:
    void dropEvent(QDropEvent *event) override
    {
        QListWidget::dropEvent(event);
        if (reordered_) reordered_();
    }
private:
    Reordered reordered_;
};

class EditorSidebar final : public QWidget {
public:
    EditorSidebar(QWidget *parent, workflow_editor_sidebar_callbacks callbacks)
        : QWidget(parent), callbacks_(std::move(callbacks))
    {
        setObjectName("workflowEditorSidebar"); setMinimumWidth(230); setMaximumWidth(300);
        setStyleSheet("QWidget#workflowEditorSidebar{background:#111820;border:1px solid #27313c;} "
                      "QLineEdit{background:#10161d;color:#e6edf3;border:1px solid #2d3946;border-radius:4px;padding:5px 8px;min-height:24px;} "
                      "QListWidget{background:#10161d;color:#dbe4ec;border:1px solid #27333f;border-radius:4px;outline:0;} "
                      "QListWidget::item{padding:5px 8px;border-radius:3px;} QListWidget::item:hover{background:#1b2834;} "
                      "QListWidget::item:selected{background:#173f66;color:#ffffff;} "
                      "QPushButton{background:#17212b;color:#dbe4ec;border:1px solid #2d3946;border-radius:4px;padding:4px 7px;min-height:26px;} "
                      "QPushButton:hover{background:#20303d;border-color:#3d5266;} QPushButton:disabled{color:#687583;background:#141b22;} "
                      "QPushButton#triggerButton{background:#246b43;border-color:#3aa86a;color:#ffffff;} "
                      "QPushButton#actionButton{background:#245d96;border-color:#347fc4;color:#ffffff;} "
                      "QLabel#heading{color:#aab6c3;font-size:11px;font-weight:700;letter-spacing:1px;}");
        auto *root = new QVBoxLayout(this); root->setContentsMargins(10,10,10,10); root->setSpacing(6);
        auto *addTitle = new QLabel("ADD NODE", this); addTitle->setObjectName("heading"); root->addWidget(addTitle);
        auto *addRow = new QHBoxLayout; addRow->setSpacing(4); addTrigger_ = button("+  Trigger Node"); addTrigger_->setObjectName("triggerButton"); addAction_ = button("+  Action Node"); addAction_->setObjectName("actionButton"); addRow->addWidget(addTrigger_); addRow->addWidget(addAction_); root->addLayout(addRow);
        auto *editRow1 = new QHBoxLayout; editRow1->setSpacing(4); edit_ = button("Edit"); copy_ = button("Copy"); paste_ = button("Paste"); editRow1->addWidget(edit_); editRow1->addWidget(copy_); editRow1->addWidget(paste_); root->addLayout(editRow1);
        auto *editRow2 = new QHBoxLayout; editRow2->setSpacing(4); duplicate_ = button("Duplicate"); remove_ = button("Delete"); editRow2->addWidget(duplicate_); editRow2->addWidget(remove_); root->addLayout(editRow2);
        auto *workflowTitle = new QLabel("WORKFLOW NODES", this); workflowTitle->setObjectName("heading"); root->addWidget(workflowTitle);
        search_ = new QLineEdit(this); search_->setPlaceholderText("Search node library..."); root->addWidget(search_);
        workflowNodes_ = new ReorderList(this); workflowNodes_->setObjectName("workflowNodesList"); workflowNodes_->setDragDropMode(QAbstractItemView::InternalMove); root->addWidget(workflowNodes_,1);
        connect(search_, &QLineEdit::textChanged, this, [this](const QString &text){ filterWorkflowNodes(text); });
        workflowNodes_->setReorderedCallback([this]{ saveNodeOrder(); });
        connect(workflowNodes_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item){ if(item&&callbacks_.select_node) callbacks_.select_node(item->data(Qt::UserRole).toByteArray().constData()); });
        connect(workflowNodes_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item){ if(!item)return; if(callbacks_.select_node)callbacks_.select_node(item->data(Qt::UserRole).toByteArray().constData()); if(callbacks_.edit_node)callbacks_.edit_node(); });
        connect(addTrigger_, &QPushButton::clicked, this, [this]{if(callbacks_.add_trigger)callbacks_.add_trigger();}); connect(addAction_, &QPushButton::clicked, this, [this]{if(callbacks_.add_node)callbacks_.add_node("action");});
        connect(edit_, &QPushButton::clicked, this, [this]{if(callbacks_.edit_node)callbacks_.edit_node();}); connect(copy_, &QPushButton::clicked, this, [this]{if(callbacks_.copy_node)callbacks_.copy_node();}); connect(paste_, &QPushButton::clicked, this, [this]{if(callbacks_.paste_node)callbacks_.paste_node();}); connect(duplicate_, &QPushButton::clicked, this, [this]{if(callbacks_.duplicate_node)callbacks_.duplicate_node();}); connect(remove_, &QPushButton::clicked, this, [this]{if(callbacks_.delete_node)callbacks_.delete_node();});
    }
    void setSelectionState(bool selected, bool paste){edit_->setEnabled(selected);copy_->setEnabled(selected);duplicate_->setEnabled(selected);remove_->setEnabled(selected);paste_->setEnabled(paste);}
    void setWorkflowId(const char *id){workflowId_=QString::fromUtf8(id?id:"");}
    void setWorkflowNodes(const QList<NodeItem *> &nodes, NodeItem *selected){
        QSignalBlocker blocker(workflowNodes_);
        if (sameNodeSet(nodes)) {
            for (int i = 0; i < workflowNodes_->count(); ++i) {
                auto *item = workflowNodes_->item(i);
                for (NodeItem *node : nodes) if (node && node->id() == item->data(Qt::UserRole).toString()) {
                    item->setText(node->nodeName());
                    item->setIcon(workflow_editor_sidebar_node_type_icon(node->workflowNode()->type,node_configured(node->workflowNode())));
                    if (node == selected) workflowNodes_->setCurrentItem(item);
                    break;
                }
            }
            return;
        }
        workflowNodes_->clear();
        const QStringList order=workflow_editor_node_order::ordered_node_ids(workflowId_,nodes);
        for(const QString &id:order) for(NodeItem *node:nodes) if(node&&node->id()==id){
            auto *item=new QListWidgetItem(workflow_editor_sidebar_node_type_icon(node->workflowNode()->type,node_configured(node->workflowNode())),node->nodeName(),workflowNodes_);
            item->setData(Qt::UserRole,node->id()); if(node==selected) workflowNodes_->setCurrentItem(item); break;
        }
        filterWorkflowNodes(search_->text());
    }
private:
    bool sameNodeSet(const QList<NodeItem *> &nodes) const
    {
        if (workflowNodes_->count() != nodes.size()) return false;
        for (NodeItem *node : nodes) {
            if (!node) continue;
            bool found = false;
            for (int i = 0; i < workflowNodes_->count(); ++i)
                if (workflowNodes_->item(i)->data(Qt::UserRole).toString() == node->id()) { found = true; break; }
            if (!found) return false;
        }
        return true;
    }
    QPushButton *button(const char *text){return new QPushButton(text,this);}
    void filterWorkflowNodes(const QString &text){workflowNodes_->setDragDropMode(text.isEmpty()?QAbstractItemView::InternalMove:QAbstractItemView::NoDragDrop);for(int i=0;i<workflowNodes_->count();++i)workflowNodes_->item(i)->setHidden(!workflowNodes_->item(i)->text().contains(text,Qt::CaseInsensitive));}
    void saveNodeOrder(){QStringList ids;for(int i=0;i<workflowNodes_->count();++i)ids.append(workflowNodes_->item(i)->data(Qt::UserRole).toString());workflow_editor_node_order::save_order(workflowId_,ids);}
    workflow_editor_sidebar_callbacks callbacks_; QString workflowId_;
    QLineEdit *search_=nullptr; ReorderList *workflowNodes_=nullptr; QPushButton *addTrigger_=nullptr,*addAction_=nullptr,*edit_=nullptr,*copy_=nullptr,*paste_=nullptr,*duplicate_=nullptr,*remove_=nullptr;
};
}
QWidget *create_workflow_editor_sidebar(QWidget *parent, workflow_editor_sidebar_callbacks callbacks){return new EditorSidebar(parent,std::move(callbacks));}
void workflow_editor_sidebar_set_selection_state(QWidget *sidebar,bool selected,bool paste){if(auto *widget=dynamic_cast<EditorSidebar*>(sidebar))widget->setSelectionState(selected,paste);}
void workflow_editor_sidebar_set_workflow_id(QWidget *sidebar,const char *id){if(auto *widget=dynamic_cast<EditorSidebar*>(sidebar))widget->setWorkflowId(id);}
void workflow_editor_sidebar_set_workflow_nodes(QWidget *sidebar,const QList<NodeItem*> &nodes,NodeItem *selected){if(auto *widget=dynamic_cast<EditorSidebar*>(sidebar))widget->setWorkflowNodes(nodes,selected);}
