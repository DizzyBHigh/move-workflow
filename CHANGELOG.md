# Changelog

## 2.0.0

This release adds a number of tools for working with, testing, and monitoring running workflows, along with a new shortcut system and better separation of workflow runtime state.

### What's New

#### Keyboard Shortcuts

- Added workflow shortcuts for triggering actions from configurable keyboard keys.
- A single shortcut can be used by multiple workflow connections, allowing several actions to respond to the same key press.
- Added shortcut configuration and editing directly in the workflow editor.
- Added handling for shortcut conflicts with hotkeys already registered elsewhere in OBS.
- Shortcut execution is handled by the workflow engine rather than relying on Move Transition's own chaining.

#### Workflow Runtime Control

- Added tracking for individual workflow runs.
- Multiple runs of the same workflow can now exist independently.
- Added the ability to stop an individual workflow run without affecting other runs.
- Added the ability to stop all active runs belonging to a specific workflow.
- Improved cleanup of temporary Move filter instances when a run ends or is stopped.

#### Run From Node

- Added play buttons to workflow nodes in the editor.
- A workflow can now be started directly from any node for testing and debugging.
- Starting a node this way follows the normal workflow timing and connection rules.

#### Active Runs Monitor

- Added an Active Runs panel to the Workflow Editor.
- The panel shows currently running workflow instances and the node they are executing.
- Added timing information for Start Delay, Execution/Duration, and End Delay.
- Added Stop Instance controls for individual runs.
- Added Stop All control for the selected workflow.
- Shortcut-waiting runs are clearly shown in the monitor.

#### Standalone Workflow Monitor

- Added a separate Move Workflow Monitor for monitoring workflows without the main Workflow Editor open.
- The monitor can view runs for a specific workflow or all workflows.
- Individual workflow instances can be stopped directly from the monitor.
- The monitor can be docked, floated, or tabbed alongside other OBS panels.

#### Workflow Node Organization

- Added drag-and-drop reordering to the Workflow Nodes list.
- The list order is independent of the workflow graph and execution order, so reorganizing the list does not change how the workflow runs.
- Sidebar node ordering is persisted separately from the workflow itself.
- Node ordering is included when exporting and importing workflows.

#### Action Editor Improvements

- Improved Move filter selection so the editor displays the filter's friendly name while retaining the actual OBS filter identity internally.
- Improved restoration of timing and easing settings when editing existing actions.
- New Action nodes now correctly start with workflow timing set to use the existing Move filter behaviour.
- Improved handling of Move filter instances during runtime execution to prevent state from leaking between workflow runs.

#### Editor Improvements

- Improved node and connection handling around scene/source relationships.
- Improved node and filter selection behaviour.
- Added visual feedback for actively executing Action nodes.

### Under the Hood

The workflow runtime has been split into smaller components for execution, delays, shortcuts, runtime filter instances, and monitoring. This makes the workflow engine easier to extend while keeping the underlying Move Transition filters responsible for what an action actually does.

Move Workflow controls when actions happen and how the workflow proceeds, while Move Transition continues to control what those actions actually do.

## 1.0.0-rc1

Initial release candidate for the Move Workflow layer.

- Workflow editor with Trigger and Action nodes
- Workflow-controlled sequential and simultaneous execution
- Move filter actions delegated to obs-move-transition
- Change Scene action
- Start Delay, Duration, and End Delay controls
- Workflow trigger filters
- Workflow import/export and persistence
- Node and connection editing
- Debug logging control
