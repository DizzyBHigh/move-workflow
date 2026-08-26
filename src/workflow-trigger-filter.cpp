#include "workflow-trigger-filter.h"
#include "workflow-engine-service.h"
#include "workflow-model.h"
#include "workflow-persistence.h"
#include <obs.h>
#include <cstring>
namespace {
struct trigger_filter { obs_source_t *source; bool visible; char workflow_id[WORKFLOW_MAX_NAME]; char trigger_id[WORKFLOW_MAX_NAME]; };
static const char *name(void *) { return "Trigger Workflow"; }
static void *create(obs_data_t *settings,obs_source_t *source){auto*d=new trigger_filter{};d->source=source;std::strncpy(d->workflow_id,obs_data_get_string(settings,"workflow"),sizeof(d->workflow_id)-1);std::strncpy(d->trigger_id,obs_data_get_string(settings,"trigger"),sizeof(d->trigger_id)-1);return d;}
static void destroy(void*p){delete static_cast<trigger_filter*>(p);}
static void update(void*p,obs_data_t*s){auto*d=static_cast<trigger_filter*>(p);if(!d)return;std::strncpy(d->workflow_id,obs_data_get_string(s,"workflow"),sizeof(d->workflow_id)-1);std::strncpy(d->trigger_id,obs_data_get_string(s,"trigger"),sizeof(d->trigger_id)-1);}
static void poll(void*p){auto*d=static_cast<trigger_filter*>(p);if(d&&!workflow_engine_service_workflow_running(d->workflow_id)){obs_source_set_enabled(d->source,false);obs_timer_remove(poll,d);}}
static void show(void*p){auto*d=static_cast<trigger_filter*>(p);if(!d||d->visible)return;d->visible=true;if(d->workflow_id[0]&&d->trigger_id[0]&&workflow_engine_service_trigger(d->workflow_id,d->trigger_id))obs_timer_add(poll,d,50);}
static void hide(void*p){auto*d=static_cast<trigger_filter*>(p);if(d)d->visible=false;}
static void fill_triggers(obs_property_t*t,const workflow_t*w){obs_property_list_clear(t);if(!w)return;for(size_t n=0;n<w->node_count;++n){const auto&node=w->nodes[n];if(node.type==WORKFLOW_NODE_TRIGGER)obs_property_list_add_string(t,node.name,node.id);}}
static bool workflow_changed(obs_properties_t*props,obs_property_t*,obs_data_t*settings){auto*m=workflow_persistence_manager();auto*w=m?workflow_manager_find(m,obs_data_get_string(settings,"workflow")):nullptr;auto*t=obs_properties_get(props,"trigger");fill_triggers(t,w);return true;}
static obs_properties_t*properties(void*){auto*p=obs_properties_create();auto*wprop=obs_properties_add_list(p,"workflow","Workflow",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_STRING);auto*tprop=obs_properties_add_list(p,"trigger","Trigger",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_STRING);obs_property_set_modified(wprop,workflow_changed);auto*m=workflow_persistence_manager();if(m&&m->workflow_count){for(size_t i=0;i<m->workflow_count;++i)obs_property_list_add_string(wprop,m->workflows[i].name,m->workflows[i].id);fill_triggers(tprop,&m->workflows[0]);}return p;}
static obs_source_info info=[](){obs_source_info v{};v.id="move_workflow_trigger_filter";v.type=OBS_SOURCE_TYPE_FILTER;v.output_flags=OBS_SOURCE_VIDEO;v.get_name=name;v.create=create;v.destroy=destroy;v.update=update;v.get_properties=properties;v.show=show;v.hide=hide;return v;}();
}
void workflow_trigger_filter_register(void){obs_register_source(&info);}
