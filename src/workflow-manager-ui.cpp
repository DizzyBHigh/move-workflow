#include "workflow-manager-ui.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QWidget>
#include <cstdio>

namespace {
class WorkflowManagerWidget final : public QWidget {
public:
    WorkflowManagerWidget(workflow_manager_t *manager, QWidget *parent,
                          std::function<void(const char *)> selectionChanged,
                          std::function<bool(const char *)> createWorkflow,
                          std::function<bool(const char *)> duplicateWorkflow,
                          std::function<bool()> deleteWorkflow,
                          std::function<bool(const char *)> exportWorkflow,
                          std::function<bool(const char *)> importWorkflow,
                          std::function<void()> stateChanged)
        : QWidget(parent), manager_(manager), selectionChanged_(std::move(selectionChanged)),
          createWorkflow_(std::move(createWorkflow)), duplicateWorkflow_(std::move(duplicateWorkflow)),
          deleteWorkflow_(std::move(deleteWorkflow)), exportWorkflow_(std::move(exportWorkflow)),
          importWorkflow_(std::move(importWorkflow)), stateChanged_(std::move(stateChanged))
    {
        auto *layout = new QHBoxLayout(this); layout->setContentsMargins(0,0,0,0);
        combo_=new QComboBox(this); add_=new QPushButton("+ New",this); duplicate_=new QPushButton("Duplicate",this);
        rename_=new QPushButton("Rename",this); remove_=new QPushButton("Delete",this);
        export_=new QPushButton("Export",this); import_=new QPushButton("Import",this); enabled_=new QCheckBox("Enabled",this);
        layout->addWidget(combo_,1); layout->addWidget(add_); layout->addWidget(duplicate_); layout->addWidget(rename_);
        layout->addWidget(remove_); layout->addWidget(export_); layout->addWidget(import_); layout->addWidget(enabled_); refresh();
        connect(combo_,&QComboBox::currentIndexChanged,this,[this](int i){if(i>=0&&selectionChanged_){const QByteArray id=combo_->itemData(i).toByteArray();selectionChanged_(id.constData());}refresh();});
        connect(add_,&QPushButton::clicked,this,[this]{addWorkflow();}); connect(duplicate_,&QPushButton::clicked,this,[this]{this->duplicateWorkflow();});
        connect(rename_,&QPushButton::clicked,this,[this]{renameWorkflow();}); connect(remove_,&QPushButton::clicked,this,[this]{removeWorkflow();});
        connect(export_,&QPushButton::clicked,this,[this]{exportWorkflow();}); connect(import_,&QPushButton::clicked,this,[this]{importWorkflow();});
        connect(enabled_,&QCheckBox::toggled,this,[this](bool e){const auto *s=workflow_manager_selected_const(manager_);if(s)workflow_manager_set_enabled(manager_,s->id,e);if(stateChanged_)stateChanged_();});
    }
    void refresh(){const auto *s=workflow_manager_selected_const(manager_);const QString id=s?QString::fromUtf8(s->id):QString();combo_->blockSignals(true);combo_->clear();for(size_t i=0;i<manager_->workflow_count;++i)combo_->addItem(QString::fromUtf8(manager_->workflows[i].name),QString::fromUtf8(manager_->workflows[i].id));int n=combo_->findData(id);combo_->setCurrentIndex(n>=0?n:0);combo_->blockSignals(false);s=workflow_manager_selected_const(manager_);bool has=s!=nullptr;duplicate_->setEnabled(has);rename_->setEnabled(has);remove_->setEnabled(has);export_->setEnabled(has);enabled_->setEnabled(has);enabled_->blockSignals(true);enabled_->setChecked(has&&s->enabled);enabled_->blockSignals(false);}
private:
    void addWorkflow(){bool ok=false;const QString n=QInputDialog::getText(this,"New Workflow","Workflow name:",QLineEdit::Normal,"New Workflow",&ok);if(ok&&!n.trimmed().isEmpty()&&createWorkflow_&&createWorkflow_(n.trimmed().toUtf8().constData()))refresh();}
    void duplicateWorkflow(){bool ok=false;const auto *s=workflow_manager_selected_const(manager_);const QString d=s?QString::fromUtf8(s->name)+" Copy":"Workflow Copy";const QString n=QInputDialog::getText(this,"Duplicate Workflow","Workflow name:",QLineEdit::Normal,d,&ok);if(ok&&!n.trimmed().isEmpty()&&duplicateWorkflow_&&duplicateWorkflow_(n.trimmed().toUtf8().constData()))refresh();}
    void renameWorkflow(){auto *s=workflow_manager_selected(manager_);if(!s)return;bool ok=false;const QString n=QInputDialog::getText(this,"Rename Workflow","Workflow name:",QLineEdit::Normal,QString::fromUtf8(s->name),&ok);if(ok&&!n.trimmed().isEmpty()){snprintf(s->name,sizeof(s->name),"%s",n.trimmed().toUtf8().constData());if(stateChanged_)stateChanged_();refresh();}}
    void removeWorkflow(){const auto *s=workflow_manager_selected_const(manager_);if(!s||!deleteWorkflow_)return;const QString name=QString::fromUtf8(s->name);if(QMessageBox::question(this,"Delete Workflow",QString("Delete '%1'?\n\nThe workflow and all of its nodes will be removed.").arg(name),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;if(deleteWorkflow_())refresh();}
    void exportWorkflow(){if(!exportWorkflow_)return;const QString file=QFileDialog::getSaveFileName(this,"Export Workflow",{},"Move Workflow (*.obsworkflow.json)");if(!file.isEmpty()&&!exportWorkflow_(file.toUtf8().constData()))QMessageBox::warning(this,"Export Workflow","The workflow could not be exported.");}
    void importWorkflow(){if(!importWorkflow_)return;const QString file=QFileDialog::getOpenFileName(this,"Import Workflow",{},"Move Workflow (*.obsworkflow.json)");if(file.isEmpty()||!importWorkflow_(file.toUtf8().constData())){if(!file.isEmpty())QMessageBox::warning(this,"Import Workflow","The workflow could not be imported.");return;}refresh();}
    workflow_manager_t *manager_;std::function<void(const char*)> selectionChanged_;std::function<bool(const char*)> createWorkflow_;std::function<bool(const char*)> duplicateWorkflow_;std::function<bool()> deleteWorkflow_;std::function<bool(const char*)> exportWorkflow_;std::function<bool(const char*)> importWorkflow_;std::function<void()> stateChanged_;
    QComboBox *combo_;QPushButton *add_;QPushButton *duplicate_;QPushButton *rename_;QPushButton *remove_;QPushButton *export_;QPushButton *import_;QCheckBox *enabled_;
};}

QWidget *create_workflow_manager_ui(workflow_manager_t *manager,QWidget *parent,std::function<void(const char*)> selectionChanged,std::function<bool(const char*)> createWorkflow,std::function<bool(const char*)> duplicateWorkflow,std::function<bool()> deleteWorkflow,std::function<bool(const char*)> exportWorkflow,std::function<bool(const char*)> importWorkflow,std::function<void()> stateChanged)
{return new WorkflowManagerWidget(manager,parent,std::move(selectionChanged),std::move(createWorkflow),std::move(duplicateWorkflow),std::move(deleteWorkflow),std::move(exportWorkflow),std::move(importWorkflow),std::move(stateChanged));}
