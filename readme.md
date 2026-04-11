# foo_openhacks

A foobar2000 enhancement component that provides UI tweaks and window management features.

## Requirements

- Windows 10 1607+
- foobar2000 v1.5+ (x86)
- foobar2000 v2.0+ (x86 / x64)

## Features

### Window Management
- **Borderless window** — Remove the window border/frame for a cleaner look
- **Draggable window** — Drag the main window from any empty area when using the borderless style
- **Resizable borderless window** — Resize the window from edges even without a title bar
- **Fullscreen mode** — Toggle fullscreen via menu or keyboard shortcut
- **Maximize / Restore** — Custom maximize and restore support for borderless windows

### UI Tweaks
- **Hide menu bar** — Show or hide the DUI menu bar
- **Hide status bar** — Show or hide the DUI status bar

### Pseudo Caption
When using the borderless window style, you can define a custom draggable region (pseudo caption) with fine-grained control over its position and size:
- Configure margins from left / top / right / bottom edges
- Optionally specify a fixed width or height

### COM Automation Interface
Exposes an `IOpenHacks` COM object (ProgID: `OpenHacks`) for scripting and integration with other components (e.g. JScript Panel, Spider Monkey Panel).

See [COM_API.md](COM_API.md) for the full API reference.

## Preferences

Settings are available under **Preferences → Display → Main window**:
- Select window frame style (Default / No border)
- Configure pseudo caption region for borderless dragging

## Main Menu Commands

The following commands are available under **View** in the foobar2000 main menu:

- Show main menu *(hidden by default)*
- Show status bar *(hidden by default)*
- Maximize *(hidden by default)*
- Restore *(hidden by default)*
- **Fullscreen**

> **Tip:** Hidden commands are not shown in the menu by default. Hold **Shift** while opening the menu to reveal all hidden commands.

## Build

### Requirements

- Visual Studio 2026 (with C++ desktop workload)
- Python 3.x
- vcpkg (bootstrapped automatically via `setup_vcpkg.py`)

### Dependencies

- [minhook](https://github.com/TsudaKageyu/minhook) >= 1.3.4 (managed via vcpkg)

### Steps

```bat
python setup_vcpkg.py
python build.py --rebuild --verbose
```

The output artifact `foo_openhacks.fb2k-component` will be generated in the project root.

## License

[MIT](LICENSE) © 2026 ohyeah
