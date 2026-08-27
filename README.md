# OBS Move Workflow

OBS Move Workflow adds a workflow layer on top of Exeldro's Move Transition filters for OBS Studio.

It lets you build reusable workflows from connected nodes. A workflow can respond to a trigger and then execute one or more Move filters and/or change the active OBS scene, with workflow-controlled timing and sequencing.

## What it does

A workflow separates **when something happens** from **what the target filter does**.

- **Trigger nodes** are entry points into a workflow.
- **Action nodes** execute an existing Move filter or change the OBS scene.
- Actions can run simultaneously or lead to other actions.
- Each action can have a Start Delay, Duration and End Delay.
- The workflow engine controls sequencing instead of relying on Move Transition's own Next Move or Simultaneous Move chaining.
- Workflows can be saved, loaded, imported and exported.

The plugin does not replace the functionality of Move Transition. Move Transition remains responsible for performing the actual movement, value change, swap or action; Move Workflow controls when that operation participates in a workflow.

## Requirements

- OBS Studio 31.x or a compatible version supported by the build.
- Exeldro's **Move Transition** plugin for Move-based actions.

Move Workflow uses the existing Move filters rather than implementing their movement functionality itself.

## How the system works

A workflow is a graph of nodes.

```text
Trigger
   │
   ├──── Action A
   │
   ├──── Action B
   │
   └──── Action C
             │
             ▼
          Next Action
```

The graph determines which actions run together and which actions follow another action.

### Trigger nodes

A Trigger node is an entry point. It does not perform a Move operation itself. It provides a point that can be activated by a **Trigger Workflow** filter.

A workflow can contain multiple Trigger nodes, allowing different external triggers to enter the same workflow at different points.

### Action nodes

Action nodes currently have two high-level action types:

- **Move** — selects a source and one of the supported Move filters attached to it.
- **Change Scene** — switches OBS to a selected scene.

For Move actions, the editor displays the Move filters attached to the selected source. The workflow therefore references the existing filter instead of recreating its settings.

Supported Move filter types include:

- Move Action
- Move Source
- Move Source Swap
- Move Value

### Action timing

Each Action node has three timing stages:

```text
Start Delay
    │
    ▼
Configure / execute target
    │
    ▼
Duration
    │
    ▼
End Delay
    │
    ▼
Workflow continues
```

**Duration is the execution time assigned to the action.** It is not simply a delay before the next node starts.

The workflow engine considers an action active during its configured duration and only allows dependent workflow actions to proceed after the action's End Delay has completed.

The action editor also exposes easing fields for planned functionality; easing is not currently implemented by the workflow engine.

## Getting started

1. Install OBS Studio and Exeldro's Move Transition plugin.
2. Install OBS Move Workflow.
3. Start OBS and open **Tools → Move Workflow**.
4. Create a workflow and give it a useful name.
5. Add Trigger and Action nodes.
6. Configure each Action node.
7. Connect the nodes to define simultaneous and next actions.
8. Add a Trigger Workflow filter to something that should start the workflow.
9. Enable the workflow/trigger and test it.

Workflows are persisted by the plugin, so they remain available when OBS is restarted.

## Creating nodes

### Creating an Action node

Add an Action node in the workflow editor and open its settings.

Choose **Move** or **Change Scene**.

#### Move

1. Select the OBS source containing the Move filter.
2. Select the Move filter you want the workflow to execute.
3. Configure Start Delay, Duration and End Delay as required.
4. Configure its Simultaneous Actions and Next Actions connections.

The filter itself remains responsible for the movement/action. The workflow only controls its participation in the workflow.

#### Change Scene

1. Select **Change Scene** as the Action Type.
2. Search for/select the target scene.
3. Configure its timing.
4. Connect its following actions if required.

Change Scene is a workflow action and does not require a Move filter.

### Creating a Trigger node

Create a Trigger node and give it a descriptive name, such as:

- `Intro`
- `Show Main Camera`
- `End Credits`

The Trigger node becomes selectable from a Trigger Workflow filter once the workflow has been saved/updated.

## Connecting nodes

The workflow editor supports different relationships between nodes.

### Simultaneous Actions

Use **Simultaneous Actions** when several actions should begin together.

For example:

```text
              ┌── Move Logo ─────┐
              │                  │
Trigger ──────┼── Move Camera ───┼── Next
              │                  │
              └── Change Scene ──┘
```

The workflow engine coordinates these actions. Do not use Move Transition's Simultaneous Move setting to implement workflow sequencing.

### Next Actions

Use **Next Actions** when an action should lead to another action after its configured Duration and End Delay.

The workflow graph decides what runs next. The target Move filter's own Next Move chain does not control workflow progression.

## Setting up triggers

The normal way to start a workflow from OBS is with the **Trigger Workflow** filter.

### Add a Trigger Workflow filter

1. Add the **Trigger Workflow** filter to an appropriate OBS source.
2. Open the filter properties.
3. Select the workflow you want to start.
4. Select the **Workflow Trigger** from that workflow.

The Workflow Trigger list is populated with the Trigger nodes belonging to the selected workflow.

The trigger filter therefore provides this relationship:

```text
OBS event / filter activation
            │
            ▼
     Trigger Workflow
            │
      Workflow + Trigger
            │
            ▼
       Trigger Node
            │
            ▼
    Workflow execution
```

## Using Exeldro's Move Action to trigger a workflow

One useful way to integrate Move Workflow with existing OBS setups is to use Exeldro's **Move Action** filter to enable a Trigger Workflow filter.

This lets an existing Move Action event become the switch that starts a workflow.

### Example

Suppose you want a button or Move Action to start a workflow called `Go Live`.

1. Create the `Go Live` workflow.
2. Add a Trigger node, for example `Start`.
3. Add the Action nodes that should run when `Start` is triggered.
4. Connect those actions in the workflow editor.
5. Add a **Trigger Workflow** filter to an OBS source.
6. In that filter, select `Go Live` and the `Start` Trigger node.
7. Add/configure an Exeldro **Move Action** filter on the same source as appropriate.
8. Configure the Move Action to perform **Filter Enable** on the Trigger Workflow filter.
9. Use the Move Action's normal activation mechanism (for example a hotkey or another supported Move Action trigger).

The resulting flow is:

```text
Move Action
    │
    │ Filter Enable
    ▼
Trigger Workflow filter
    │
    ▼
Go Live / Start
    │
    ▼
Workflow Engine
    │
    ├── Action 1
    ├── Action 2
    └── Action 3
```

The Move Action is **not** responsible for sequencing the workflow. It is simply one way to activate the Trigger Workflow filter. Once the trigger fires, Move Workflow takes control of workflow sequencing.

For details of the Move Action's available actions and activation methods, refer to Exeldro's Move Transition documentation.

## Workflow sequencing vs. Move Transition sequencing

This distinction is important.

**Move Transition** controls what an individual Move filter does.

**Move Workflow** controls when that filter participates in the workflow and what workflow node runs next.

When a Move filter is executed by the workflow engine, workflow sequencing takes precedence. The workflow should therefore be built using the workflow editor's node connections rather than Move Transition's Simultaneous Move or Next Move chaining.

## Import and export

Workflows can be exported to `.obsworkflow.json` files and imported into another OBS installation or configuration.

If an imported workflow has the same name as an existing workflow, the imported copy is given a `- Copy` suffix so that the existing workflow is not overwritten.

## Persistence

Workflows are stored by the plugin in its OBS plugin configuration area and are automatically restored when OBS starts.

Workflows are independent of OBS Scene Collections and Profiles. A workflow's saved definition remains available until it is deleted from the workflow manager or the plugin configuration is removed.

Action nodes reference OBS scenes, sources and filters by their configured names. If a referenced target no longer exists, the action cannot execute; the workflow can still continue according to its configured timing and graph.

## Debugging

The workflow editor provides a **Debug** option for enabling workflow debug logging.

When enabled, the OBS log can show workflow execution information such as trigger activation, node execution and workflow graph decisions.

This is useful when checking whether a trigger fired, whether an action target exists, and why the workflow moved to its next node.

## Important behaviour

- A workflow can execute multiple actions simultaneously.
- An Action node remains active for its configured Duration.
- End Delay occurs after the action's execution duration, including when an action cannot be executed because its target is missing.
- Missing Move filters or scenes do not cause the workflow to stop indefinitely; the workflow can continue through its configured graph after the node's timing has completed.
- Move Transition remains responsible for the actual Move operation.
- Workflow sequencing is controlled by the workflow graph, not by Move Transition's Next Move or Simultaneous Move settings.
- Easing and Easing Function are reserved for future implementation.

## Credits

OBS Move Workflow is an architectural workflow layer built around Exeldro's Move Transition plugin and its existing Move filters.

- OBS Studio — https://obsproject.com/
- Move Transition by Exeldro — https://github.com/exeldro/obs-move-transition

## License

See the repository license and the applicable licenses of the OBS Studio and Move Transition components used by the project.
