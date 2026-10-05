# Selection aspect ratio

The capture toolbar has a **Selection Aspect Ratio** button. It is enabled for
new configurations; existing users can enable it in **Configuration → Interface
→ Button Selection** without changing the rest of their toolbar.

Click the button to choose **Free**, **1:1**, **4:3**, **3:2**, **16:9**, **9:16**,
or **Current**. Current snapshots the selected area's ratio and is unavailable
before an area has been selected. Changing a preset reshapes the existing area
around its centre, preserving its width where the capture bounds allow it.

The lock applies to new selections, all eight resize handles, keyboard resizing,
and Select All. Moving the selection keeps its size and stops at the capture
boundary. Resizing against that boundary scales both dimensions together rather
than clipping one dimension. Integer screenshot dimensions approximate the ratio
within pixel rounding; very small selections may be limited by the 1px minimum.

Scroll over the button to cycle through Free and the five fixed presets. Its
icon shows the fixed ratio, and the tooltip identifies the active mode. The
button can also be assigned a shortcut in the standard shortcut settings.

In Free mode, Ctrl temporarily preserves the ratio at the start of a resize
(or creates a square for a new selection). A selected preset takes precedence
over Ctrl. Shift keeps the centre fixed while resizing. The lock survives
clearing/reselecting an area within the current capture, and every new capture
starts in Free mode. It is never written to the configuration file.

## Implementation

`SelectionWidget` owns the ratio and captures the geometry at mouse press.
`SelectionGeometry` contains the shared anchor, rounding and boundary math for
mouse and keyboard changes. `AspectRatioTool` is only a non-selectable action
adapter for the existing toolbar/configuration registry: it has no drawing,
geometry or persistent lock state. Its enum value is appended, keeping all
existing button IDs stable. Opening its menu leaves the current annotation tool
selected and does not add a drawing operation to undo history.

Geometry animations apply the final rectangle immediately while a ratio is
locked so interpolation cannot temporarily change the ratio.

## Regression tests

Full build (Qt 6 Widgets and Test required):

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The geometry/widget tests can also be built independently of the optional UI
libraries:

```sh
cmake -S tests -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Tests use Qt's offscreen platform and exercise all ratios, capture bounds with
negative origins, eight handles, axis crossing, symmetric resizing, new-area
drags, temporary Ctrl, keyboard changes, external geometry updates, invalid
ratios and capture-session reset.

Manual platform checks: verify initial selection, corners/edges, Ctrl/Shift,
keyboard resizing, menu cancellation, wheel cycling, copy/save dimensions,
existing customized toolbars, and captures on monitors with different DPI.
