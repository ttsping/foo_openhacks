# foo_openhacks COM Automation Interface

The component exposes an `IOpenHacks` COM automation object for scripting and integration with other foobar2000 scripting components (e.g. JScript Panel, Spider Monkey Panel).

- **ProgID:** `OpenHacks`
- **Interface:** `IOpenHacks` (dual / IDispatch)
- **CLSID:** `{7c87ccb2-aceb-4698-b31e-30855a6787ac}`
- **IID:** `{5faf8474-cde1-4fd4-8151-6ced18b7039b}`

## Quick Start

```js
var oh = new ActiveXObject("OpenHacks");

// Toggle fullscreen
oh.ToggleFullscreen();

// Hide the menu bar
oh.MenuBarVisible = false;

// Set borderless window style
oh.WindowFrameStyle = 1;
```

## Properties

### Read-only

| Property | Type | Description |
|---|---|---|
| `DPI` | Long | Current DPI of the foobar2000 main window |

### Window Visibility

| Property | Type | Description |
|---|---|---|
| `MenuBarVisible` | Bool | `true` = menu bar shown, `false` = hidden |
| `StatusBarVisible` | Bool | `true` = status bar shown, `false` = hidden |

### Window State

| Property | Type | Description |
|---|---|---|
| `Fullscreen` | Bool | `true` = fullscreen mode active |
| `WindowState` | Long | Current window state (see values below) |
| `WindowFrameStyle` | Long | Window frame/border style (see values below) |

**WindowState values:**

| Value | Constant | Description |
|---|---|---|
| `0` | Normal | Normal window |
| `1` | Minimized | Window is minimized |
| `2` | Maximized | Window is maximized |
| `3` | Fullscreen | Window is in fullscreen mode |

**WindowFrameStyle values:**

| Value | Description |
|---|---|
| `0` | Default — standard window with title bar and border |
| `1` | No border — borderless window (enables pseudo caption dragging) |

### Pseudo Caption

The pseudo caption defines a custom draggable region on the borderless window. Only effective when `WindowFrameStyle = 1`.

#### Margin values

| Property | Type | Description |
|---|---|---|
| `PseudoCaptionLeft` | Long | Distance from the left edge (pixels) |
| `PseudoCaptionTop` | Long | Distance from the top edge (pixels) |
| `PseudoCaptionRight` | Long | Distance from the right edge (pixels) |
| `PseudoCaptionBottom` | Long | Distance from the bottom edge (pixels) |
| `PseudoCaptionWidth` | Long | Fixed width of the caption region (pixels); used when left or right anchor is disabled |
| `PseudoCaptionHeight` | Long | Fixed height of the caption region (pixels); used when top or bottom anchor is disabled |

#### Margin anchor switches

| Property | Type | Description |
|---|---|---|
| `PseudoCaptionLeftEnabled` | Bool | Enable the left margin anchor |
| `PseudoCaptionTopEnabled` | Bool | Enable the top margin anchor |
| `PseudoCaptionRightEnabled` | Bool | Enable the right margin anchor |
| `PseudoCaptionBottomEnabled` | Bool | Enable the bottom margin anchor |

> When both left and right anchors are enabled, the region spans the full width minus the margins.  
> When only one side is anchored, `PseudoCaptionWidth` is used to determine the region width.  
> The same logic applies to top/bottom anchors and `PseudoCaptionHeight`.

## Methods

| Method | Description |
|---|---|
| `ToggleMenuBar()` | Toggle the visibility of the menu bar |
| `ToggleStatusBar()` | Toggle the visibility of the status bar |
| `ToggleFullscreen()` | Toggle fullscreen mode on/off |

## Examples

### Toggle fullscreen on double-click (JScript Panel)

```js
var oh = new ActiveXObject("OpenHacks");

function on_mouse_lbtn_dblclk(x, y, mask) {
    oh.ToggleFullscreen();
}
```

### Set up a borderless window with a top drag region

```js
var oh = new ActiveXObject("OpenHacks");

// Use borderless style
oh.WindowFrameStyle = 1;

// Define a 32px tall drag region at the top, spanning full width
oh.PseudoCaptionLeftEnabled  = true;
oh.PseudoCaptionRightEnabled = true;
oh.PseudoCaptionTopEnabled   = true;
oh.PseudoCaptionBottomEnabled = false;
oh.PseudoCaptionLeft   = 0;
oh.PseudoCaptionTop    = 0;
oh.PseudoCaptionRight  = 0;
oh.PseudoCaptionHeight = 32;
```
