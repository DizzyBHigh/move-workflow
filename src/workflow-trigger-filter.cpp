#include "workflow-trigger-filter.h"
#include "workflow-engine-service.h"
#include "workflow-model.h"
#include "workflow-persistence.h"
#include <obs.h>
#include <cstring>
namespace {
struct trigger_filter { obs_source_t *source; bool visible; char workflow_id[WORKFLOW_MAX_NAME]; char trigger_id[WORKFLOW_MAX_NAME]; };
static const char *name(void *) { return "Trigger Workflow"; }
static void *create(obs_data_t *settings, obs_source_t *source){auto *d=new trigger_filter{};d->source=source;const char *w=obs_data_get_string(settings,"workflow");const char *t=obs_data_get_string(settings,"trigger");if(w)std::strncpy(d->workflow_id,w,sizeof(d->workflow_id)-1);if(t)std::strncpy(d->trigger_id,t,sizeof(d->trigger_id)-1);return d;}
static void destroy(void *opaque){delete static_cast<trigger_filter *>(opaque);}
static void update(void *opaque,obs_data_t *settings){auto *d=static_cast<trigger_filter *>(opaque);if(!d)return;std::strncpy(d->workflow_id,obs_data_get_string(settings,"workflow"),sizeof(d->workflow_id)-1);std::strncpy(d->trigger_id,obs_data_get_string(settings,"trigger"),sizeof(d->trigger_id)-1);}
static void poll(void *opaque){auto *d=static_cast<trigger_filter *>(opaque);if(d&&!workflow_engine_service_workflow_running(d->workflow_id)){obs_source_set_enabled(d->source,false);obs_timer_remove(poll,d);}}
static void show(void *opaque){auto *d=static_cast<trigger_filter *>(opaque);if(!d||d->visible)return;d->visible=true;if(!d->workflow_id[0]||!d->trigger_id[0])return;if(workflow_engine_service_trigger(d->workflow_id,d->trigger_id))obs_timer_add(poll,d,50);}
static void hide(void *opaque){auto *d=static_cast<trigger_filter *>(opaque);if(d)d->visible=false;}
static obs_properties_t *properties(void *){auto *p=obs_properties_create();auto *w=obs_properties_add_list(p,"workflow","Workflow",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_STRING);auto *t=obs_properties_add_list(p,"trigger","Trigger",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_STRING);auto *m=workflow_persistence_manager();if(m)for(size_t i=0;i<m->workflow_count;++i){const auto &wf=m->workflows[i];obs_property_list_add_string(w,wf.name,wf.id);for(size_t n=0;n<wf.node_count;++n){const auto &node=wf.nodes[n];if(node.type==WORKFLOW_NODE_TRIGGER)obs_property_list_add_string(t,node.name,node.id);}}return p;}
static obs_source_info info=[](){obs_source_info v{};v.id="move_workflow_trigger_filter";v.type=OBS_SOURCE_TYPE_FILTER;v.output_flags=OBS_SOURCE_VIDEO;v.get_name=name;v.create=create;v.destroy=destroy;v.update=update;v.get_properties=properties;v.show=show;v.hide=hide;return v;}();
}
void workflow_trigger_filter_register(void){obs_register_source(&info);}
