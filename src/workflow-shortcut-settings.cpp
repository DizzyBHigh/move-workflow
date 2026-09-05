#include "workflow-shortcut-settings.h"
#include "workflow-node.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <cstdio>
#include <cstring>

namespace workflow_shortcut_settings {
static const workflow_shortcut_binding_t *find_binding(const workflow_node_t *source,const char *target)
{
    if(!source||!target)return nullptr;
    for(size_t i=0;i<source->shortcut_binding_count;++i)
        if(std::strcmp(source->shortcut_bindings[i].target_id,target)==0)return &source->shortcut_bindings[i];
    return nullptr;
}
QWidget *create_editor(const workflow_node_t *source,const QList<NodeItem *> &nodes,QWidget *parent)
{
    auto *box=new QGroupBox("Shortcut Keys",parent);
    auto *layout=new QVBoxLayout(box);
    layout->setContentsMargins(8,8,8,8);
    auto *form=new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->addLayout(form);
    int rows=0;
    if(source){
        for(size_t i=0;i<source->shortcut_node_count;++i){
            const char *target=source->shortcut_node_ids[i];
            NodeItem *node=nullptr;
            for(NodeItem *candidate:nodes){
                if(candidate&&candidate->id().compare(QString::fromUtf8(target),Qt::CaseInsensitive)==0){node=candidate;break;}
            }
            if(!node)continue;
            auto *edit=new QKeySequenceEdit(box);
            edit->setObjectName("shortcutKey_"+node->id());
            if(const auto *binding=find_binding(source,target))
                edit->setKeySequence(QKeySequence::fromString(QString::fromUtf8(binding->key),QKeySequence::PortableText));
            edit->setToolTip(QString("Shortcut for %1").arg(node->nodeName()));
            form->addRow(node->nodeName(),edit);
            ++rows;
        }
    }
    if(rows==0)
        layout->addWidget(new QLabel("No shortcut actions configured.",box));
    box->setMinimumHeight(rows?70:48);
    return box;
}
bool read(const QWidget *editor,QList<Binding> &bindings)
{
    if(!editor)return false;bindings.clear();
    for(auto *edit:editor->findChildren<QKeySequenceEdit *>()){
        const QString name=edit->objectName();if(!name.startsWith("shortcutKey_"))continue;
        Binding binding;binding.target_id=name.mid(QString("shortcutKey_").size());
        binding.key=edit->keySequence().toString(QKeySequence::PortableText);bindings.append(binding);
    }
    return true;
}
bool apply(const QList<Binding> &bindings,workflow_node_t *source)
{
    if(!source)return false;
    for(size_t i=0;i<source->shortcut_binding_count;++i)source->shortcut_bindings[i].key[0]='\0';
    for(const Binding &binding:bindings){
        const QByteArray targetId=binding.target_id.toUtf8();auto *entry=const_cast<workflow_shortcut_binding_t *>(find_binding(source,targetId.constData()));
        if(!entry)continue;
        std::snprintf(entry->key,WORKFLOW_MAX_NAME,"%s",binding.key.toUtf8().constData());
    }
    return true;
}
} // namespace workflow_shortcut_settings
