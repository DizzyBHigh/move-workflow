# Changelog

## 2.0.0

This release adds features for testing and monitoring workflows, a new shortcut system for manually controlling when nodes advance, and better separation of workflow and runtime state.

### What's New

#### Keyboard Shortcuts

- Added a new shortcut node connector that triggers nodes using configurable keybindings.
- A single shortcut can be used by multiple workflow connections, allowing multiple nodes to be triggered by the shortcut.
- Added handling for shortcut conflicts with hotkeys already registered elsewhere in OBS.

#### Multiple Workflow handling

- Added monitoring for individual workflow runs.
- Multiple runs of the same workflow can now exist independently.
  - Stop an individual workflow run without affecting other runs.
  - Stop all active runs belonging to a specific workflow.
  - Improved cleanup of temporary Move filter instances when a run ends or is stopped.

#### Run From Node

- Added play buttons to workflow nodes in the editor.
- A workflow can now be started directly from any node for testing and debugging.

#### Workflow Monitor

- Added an Active Runs panel to the Workflow Editor.
- The panel shows currently running workflow instances and the node they are executing along with timing for Start Delay, Execution/Duration, and End Delay.
- Stop Instance controls for individual runs.
- Stop All control for the selected workflow.
- Runs waiting for a shortcut key are shown in the monitor.

#### Standalone Workflow Monitor

- Added a dockable Move Workflow Monitor for monitoring workflows without the main Workflow Editor open.

#### Workflow Node Organization

- Added drag-and-drop reordering to the Workflow Nodes list.
- Nodes list ordering is persisted separately from the workflow itself.
- Node ordering is included when exporting and importing workflows.

#### Action Editor Improvements

- Improved restoration of timing and easing settings when editing existing actions.
- New Action nodes now correctly start with workflow timing set to use the existing Move filter behaviour.
- Improved handling of Move filters during execution to prevent settings from leaking between workflow runs.

#### Editor Improvements

- Improved node and connection handling around scene/source relationships.
- Improved node and filter selection behaviour.
- Added visual feedback for actively executing Action nodes.

### Behind-the-scenes changes

The workflow runtime has been split into smaller components for execution, delays, shortcuts, runtime filter instances, and monitoring. 

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
