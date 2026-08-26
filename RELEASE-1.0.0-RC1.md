# obs-move-workflow 1.0.0-rc1

This release candidate freezes the workflow editor and execution model for the 1.0.0 release.

## Release candidate focus

- Workflow editor UI and node editing
- Trigger nodes and workflow trigger filters
- Move filter actions delegated to obs-move-transition
- Change Scene actions
- Sequential and simultaneous workflow connections
- Start Delay, Duration, and End Delay
- Workflow import/export and persistence
- Workflow debug logging

## Testing required before 1.0.0

- Clean Windows build and installation
- Save/restart/reload workflow persistence
- Import/export round trip
- Move Action, Move Source, Move Source Swap, and Move Value filters
- Change Scene action
- Sequential and simultaneous execution
- Missing filter/scene references
- Repeated workflow execution
- OBS/editor shutdown stability

## Version

`1.0.0-rc1`
