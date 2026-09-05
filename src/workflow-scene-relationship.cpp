#include "workflow-scene-relationship.h"
#include <cstdio>
#include <cstring>

namespace workflow_scene_relationship {
static bool contains(size_t count,const char ids[][WORKFLOW_MAX_NAME],const char *id)
{
    if(!id||!*id)return false;
    for(size_t i=0;i<count;++i)if(std::strcmp(ids[i],id)==0)return true;
    return false;
}
static bool append(size_t &count,char ids[][WORKFLOW_MAX_NAME],const char *id)
{
    if(!id||!*id||count>=WORKFLOW_MAX_LINKS||contains(count,ids,id))return false;
    std::snprintf(ids[count],WORKFLOW_MAX_NAME,"%s",id);++count;return true;
}
static void erase(size_t &count,char ids[][WORKFLOW_MAX_NAME],const char *id)
{
    if(!id)return;
    for(size_t i=0;i<count;++i)if(std::strcmp(ids[i],id)==0){for(size_t j=i+1;j<count;++j)std::memcpy(ids[j-1],ids[j],WORKFLOW_MAX_NAME);--count;return;}
}
static bool add_binding(workflow_node_t *source,const char *target)
{
    if(!source||!target||source->shortcut_binding_count>=WORKFLOW_MAX_LINKS)return false;
    for(size_t i=0;i<source->shortcut_binding_count;++i)
        if(std::strcmp(source->shortcut_bindings[i].target_id,target)==0)return true;
    auto &binding=source->shortcut_bindings[source->shortcut_binding_count++];
    std::snprintf(binding.target_id,WORKFLOW_MAX_NAME,"%s",target);binding.key[0]='\0';return true;
}
static void remove_binding(workflow_node_t *source,const char *target)
{
    if(!source||!target)return;
    for(size_t i=0;i<source->shortcut_binding_count;++i)if(std::strcmp(source->shortcut_bindings[i].target_id,target)==0){for(size_t j=i+1;j<source->shortcut_binding_count;++j)std::memcpy(&source->shortcut_bindings[j-1],&source->shortcut_bindings[j],sizeof(source->shortcut_bindings[0]));--source->shortcut_binding_count;return;}
}
bool add(workflow_node_t *source,workflow_node_t *target,const QString &type)
{
    if(!source||!target||source==target)return false;
    if(type=="Shortcut"){
        if(contains(source->shortcut_node_count,source->shortcut_node_ids,target->id)||source->shortcut_binding_count>=WORKFLOW_MAX_LINKS)return false;
        if(!append(source->shortcut_node_count,source->shortcut_node_ids,target->id)||!add_binding(source,target->id))return false;
        return true;
    }
    if(type=="Simultaneous")return append(source->simultaneous_node_count,source->simultaneous_node_ids,target->id);
    if(type=="Next"||type=="Next Action")return append(source->next_node_count,source->next_node_ids,target->id);
    return false;
}
bool remove(workflow_node_t *source,workflow_node_t *target,const QString &type)
{
    if(!source||!target)return false;
    if(type=="Shortcut"){erase(source->shortcut_node_count,source->shortcut_node_ids,target->id);remove_binding(source,target->id);}
    else if(type=="Simultaneous")erase(source->simultaneous_node_count,source->simultaneous_node_ids,target->id);
    else if(type=="Next"||type=="Next Action")erase(source->next_node_count,source->next_node_ids,target->id);
    else return false;
    return true;
}
bool has(const workflow_node_t *source,const char *target_id,const QString &type)
{
    if(!source)return false;
    if(type=="Shortcut")return contains(source->shortcut_node_count,source->shortcut_node_ids,target_id);
    if(type=="Simultaneous")return contains(source->simultaneous_node_count,source->simultaneous_node_ids,target_id);
    if(type=="Next"||type=="Next Action")return contains(source->next_node_count,source->next_node_ids,target_id);
    return false;
}
QString type_between(const workflow_node_t *source,const workflow_node_t *target)
{
    if(!source||!target)return QString();
    if(contains(source->shortcut_node_count,source->shortcut_node_ids,target->id))return "Shortcut";
    if(contains(source->simultaneous_node_count,source->simultaneous_node_ids,target->id))return "Simultaneous";
    if(contains(source->next_node_count,source->next_node_ids,target->id))return "Next Action";
    return QString();
}
} // namespace workflow_scene_relationship
