# OBS Move Workflow

![OBS Move Workflow](images/obs-move-workflow%20Repo%20Image.jpg)

OBS Move Workflow adds a workflow system to OBS Studio that works with the Move filters from [Exeldro's Move Transition plugin](https://github.com/exeldro/obs-move-transition).

The idea is simple: Move Transition handles what happens to a source, while Move Workflow handles when it happens and what should happen next.

You build a workflow in the editor by adding Trigger and Action nodes and connecting them together. A trigger starts the workflow, and the workflow engine then runs the actions in the order you have defined.

## What it does

A workflow can:

- Start from a Trigger Workflow filter.
- Run existing Move filters attached to OBS sources.
- Change the active OBS scene.
- Run several actions at the same time.
- Run actions after another action has finished.
- Add a Start Delay, Duration and End Delay to each action.
- Save workflows between OBS restarts.
- Export and import workflows as `.obsworkflow.json` files.

Move Workflow does not replace [Move Transition](https://github.com/exeldro/obs-move-transition). Move Transition still performs the actual move, value change, source swap or Move Action. Move Workflow controls when that filter is run and how it fits into the workflow.

## Requirements

- OBS Studio 31.x or a compatible version supported by the build.
- [Exeldro's Move Transition plugin](https://github.com/exeldro/obs-move-transition) for Move actions.

## How it works

A workflow is a graph made up of nodes.

A Trigger node is an entry point into the workflow. It starts the actions connected to it.

An Action node does the work. It can either run a Move filter or change the current OBS scene.

The connections between nodes tell the workflow engine what to do next. This means you do not need to use [Move Transition](https://github.com/exeldro/obs-move-transition)'s Next Move or Simultaneous Move settings to build your workflow.

### Action types

Action nodes have two choices:

**Move**

Select an OBS source and then select one of the supported Move filters attached to that source. The workflow uses the existing filter and does not recreate its settings.

Supported Move filters include:

- Move Action
- Move Source
- Move Source Swap
- Move Value

**Change Scene**

Select the OBS scene that should become active when the action runs. The scene list can be searched in the node editor.

### Action timing

Each Action node has three timing values:

1. Start Delay - how long to wait before starting the action.
2. Duration - how long the action is considered to be running.
3. End Delay - how long to wait after the action has finished before the workflow moves on.

Duration is the time assigned to the action itself. It is not just a delay before the next node starts.

If an action cannot be run because its Move filter or scene no longer exists, its End Delay is still applied before the workflow continues.

Easing and Easing Function are shown in the workflow data for future use but are not implemented in 1.0.0.

## Getting started

1. Install OBS Studio.
2. Install [Exeldro's Move Transition plugin](https://github.com/exeldro/obs-move-transition) if you want to use Move actions.
3. Install OBS Move Workflow.
4. Start OBS and open **Tools > Move Workflow**.
5. Create a workflow and give it a useful name.
6. Add a Trigger node.
7. Add the Action nodes you need.
8. Connect the nodes to define how the workflow should run.
9. Add a Trigger Workflow filter to something that should start the workflow.
10. Select the workflow and Trigger node in that filter.
11. Test the workflow.

Workflows are saved by the plugin and are restored when OBS starts.

## Creating nodes

The workflow editor provides controls for adding nodes. New nodes can then be moved around the editor to make the workflow easy to read.

### Trigger node

Add a Trigger node and give it a name that describes what it starts. For example:

- Start Intro
- Show Camera
- End Credits

The name is also used to identify the trigger when you configure a Trigger Workflow filter.

A workflow can have more than one Trigger node. This is useful when different triggers should enter the same workflow at different points.

### Action node

Add an Action node and open its settings.

Choose **Move** or **Change Scene**.

For a Move action:

1. Select the OBS source containing the filter.
2. Select the Move filter.
3. Set the Start Delay, Duration and End Delay if required.
4. Set up its Simultaneous Actions and Next Actions connections.

For a Change Scene action:

1. Select **Change Scene**.
2. Search for and select the target scene.
3. Set the timing if required.
4. Set up any following connections.

## Connecting nodes

Connections are made directly in the workflow editor.

To create a connection, drag from the connection point at the bottom of a node and release it over the node you want to connect to.

After you release the connection, the editor will ask which type of connection you want to create. Select the connection type that matches how you want the target node to run.

### Simultaneous Actions

Use a Simultaneous Action connection when several actions should start together.

For example, one Trigger can start a camera move, a logo move and another action at the same time.

The workflow engine coordinates these actions. Do not use [Move Transition](https://github.com/exeldro/obs-move-transition)'s Simultaneous Move setting to control workflow sequencing.

### Next Actions

Use a Next Action connection when another action should start after the current action has completed its Duration and End Delay.

The workflow graph decides what runs next. [Move Transition](https://github.com/exeldro/obs-move-transition)'s Next Move setting does not control the workflow.

## Setting up a trigger

The Trigger Workflow filter is the normal way to start a workflow from OBS.

1. Add a **Trigger Workflow** filter to an OBS source.
2. Open the filter properties.
3. Select the workflow you want to run.
4. Select the Trigger node that should start the workflow.

The Workflow Trigger list is filled with the Trigger nodes from the selected workflow.

Once the Trigger Workflow filter is activated, the selected Trigger node is passed to the workflow engine and the connected actions begin according to the workflow graph.

## Using Move Action to trigger a workflow

[Exeldro's Move Action](https://github.com/exeldro/obs-move-transition) filter can be used to activate a Trigger Workflow filter. This is a useful way to connect an existing OBS setup to a workflow.

For example, you could use a Move Action hotkey to start a workflow called `Go Live`.

1. Create a `Go Live` workflow.
2. Add a Trigger node called `Start`.
3. Add and configure the Action nodes that should run.
4. Connect the nodes in the workflow editor.
5. Add a **Trigger Workflow** filter to the appropriate OBS source.
6. Set the Trigger Workflow filter to `Go Live` and `Start`.
7. Add an [Exeldro Move Action](https://github.com/exeldro/obs-move-transition) filter where you want the external trigger to originate.
8. Configure the Move Action to use **Filter Enable** on the Trigger Workflow filter.
9. Use the Move Action's normal trigger, such as its hotkey, to activate the workflow.

The sequence is:

Move Action enables the Trigger Workflow filter, the Trigger Workflow filter activates the selected Trigger node, and the workflow engine runs the connected actions.

Move Action is not controlling the workflow sequence. It is simply being used as the trigger that starts it.

See [Exeldro's Move Transition repository](https://github.com/exeldro/obs-move-transition) for the other actions and trigger methods provided by Move Action.

## Workflow sequencing

The workflow graph is responsible for deciding what runs together and what runs next.

Move Transition is responsible for performing the operation on the target. For example, Move Source moves a source and Move Value changes a value.

This separation is important when building a workflow. Configure the actual movement or action in the Move filter, and use the workflow editor to decide when that filter runs and what follows it.

## Import and export

Workflows can be exported from the workflow manager to `.obsworkflow.json` files and imported into another OBS installation or configuration.

If the imported workflow has the same name as an existing workflow, the imported workflow is given a `- Copy` suffix rather than replacing the existing workflow.

## Persistence

Workflows are stored in the plugin's OBS configuration area and are loaded again when OBS starts.

Workflows are separate from OBS Profiles and Scene Collections. They remain available until they are deleted from the workflow manager or the plugin configuration is removed.

Action nodes store the names of their target scenes, sources and filters. If a target has been removed, that action cannot run, but the workflow can still continue according to its timing and connections.

## Debugging

The workflow editor has a **Debug** checkbox. Enable it when you need more information in the OBS log while testing a workflow.

Debug logging can show trigger activation, node execution and decisions made by the workflow engine. This can help identify whether a trigger fired or whether an action target could not be found.

## Notes for 1.0.0

- Multiple actions can run at the same time.
- An Action node remains active for its configured Duration.
- End Delay is applied even when an action cannot find its target.
- Workflow sequencing is controlled by node connections, not by [Move Transition](https://github.com/exeldro/obs-move-transition)'s Next Move or Simultaneous Move settings.
- Easing is not implemented yet.

## Credits

OBS Move Workflow is built as a workflow layer around [Exeldro's Move Transition plugin](https://github.com/exeldro/obs-move-transition) and its existing Move filters.

- OBS Studio: https://obsproject.com/
- Move Transition by Exeldro: https://github.com/exeldro/obs-move-transition

## License

OBS Move Workflow is licensed under the **GNU General Public License version 2 (GPL-2.0)**.

You can find the full license text in the [LICENSE](LICENSE) file in this repository.

This project also uses OBS Studio and [Exeldro's Move Transition plugin](https://github.com/exeldro/obs-move-transition). Those projects have their own licenses and copyright notices.
