# To-Do List Manager (Win32 GUI + Assembly)

A feature-rich Windows desktop application for managing multiple to-do lists, built with C and inline x86 Assembly using the Win32 API.

## 📋 Features

- **Multiple Lists**: Create and manage multiple separate to-do lists
- **Task Management**: Add, complete, and delete tasks with deadlines
- **Automatic Sorting**: Tasks automatically sort by deadline (overdue tasks highlighted)
- **Persistent Storage**: Data automatically saves to file and loads on startup
- **Assembly Integration**: Core arithmetic operations implemented in x86 assembly
- **Native Windows UI**: Clean, responsive Win32 interface with listboxes and buttons

## 🛠️ Technical Details

### Architecture
- **Language**: C (C99 compatible)
- **GUI Framework**: Win32 API (native Windows)
- **Assembly**: Inline x86 assembly (MSVC and GCC syntax supported)
- **File Format**: Binary `.dat` file for data persistence

### Data Structures
```c
- Maximum 20 lists (folders)
- Maximum 50 tasks per list
- Task fields: description, deadline (YYYY-MM-DD), completion status
- Automatic deadline parsing and sorting
```

### Assembly Functions
Three core operations use inline assembly:
- `asm_add()` - Addition
- `asm_subtract()` - Subtraction
- `asm_increment()` - Increment by 1

These functions automatically adapt to your compiler:
- **MSVC**: Uses `__asm` syntax
- **GCC/MinGW**: Uses `__asm__` syntax with AT&T notation
- **Other**: Falls back to standard C operations

## 📦 Requirements

### Windows Build
- **OS**: Windows 7 or later
- **Compiler**: One of the following:
  - Visual Studio 2015 or later (MSVC)
  - MinGW-w64 (GCC for Windows)
  - TDM-GCC
  - Clang for Windows

### Linux Cross-Compilation (Advanced)
- MinGW cross-compiler: `mingw-w64`
- Wine (for testing)

## 🔨 Compilation Instructions

### Method 1: Visual Studio Command Line (MSVC)

Open "Developer Command Prompt for VS" and run:

```bash
cl todo_manager_win32.c /Fe:TodoManager.exe user32.lib gdi32.lib comctl32.lib
```

**Flags explained:**
- `/Fe:` - Specifies output executable name
- `user32.lib` - Windows User Interface API
- `gdi32.lib` - Graphics Device Interface (for drawing)
- `comctl32.lib` - Common Controls (for modern UI elements)

### Method 2: MinGW / MinGW-w64 (GCC)

```bash
gcc todo_manager_win32.c -o TodoManager.exe -mwindows -lcomctl32 -lgdi32 -luser32
```

**Flags explained:**
- `-mwindows` - Build as Windows GUI application (not console)
- `-lcomctl32` - Link common controls library
- `-lgdi32` - Link graphics library
- `-luser32` - Link user interface library

### Method 3: Visual Studio IDE

1. Open Visual Studio
2. **File → New → Project**
3. Select "Empty Project" (C++)
4. Add `todo_manager_win32.c` to Source Files
5. Right-click project → **Properties**
   - Configuration Properties → Linker → System
   - SubSystem: **Windows (/SUBSYSTEM:WINDOWS)**
6. **Build → Build Solution** (F7)
7. Executable will be in `Debug/` or `Release/` folder

### Method 4: Code::Blocks

1. Create new "Win32 GUI project"
2. Replace main file with `todo_manager_win32.c`
3. **Build → Build** (Ctrl+F9)

### Method 5: Cross-Compile from Linux

```bash
# Install MinGW cross-compiler
sudo apt-get install mingw-w64

# Compile for Windows
x86_64-w64-mingw32-gcc todo_manager_win32.c -o TodoManager.exe -mwindows -lcomctl32 -lgdi32 -luser32
```

## 🚀 Running the Application

### First Run
1. Double-click `TodoManager.exe`
2. If no saved data exists, you'll start with an empty application

### Basic Workflow
1. **Create a List**
   - Type list name in "New List Name" field
   - Click "Create List" button
   - New list will be automatically selected

2. **Add Tasks**
   - Select a list from the left panel
   - Enter task description
   - Enter deadline in `YYYY-MM-DD` format (e.g., `2025-12-31`)
   - Click "Add Task"

3. **Manage Tasks**
   - Click a task to select it
   - Click "Complete Task" to mark as done
   - Click "Delete Task" to remove it
   - Completed tasks move to bottom automatically

4. **Save/Load**
   - Data auto-saves when closing the application
   - Manual save: Click "Save Data" button
   - Manual load: Click "Load Data" button

### Data File
- File name: `todo_data.dat`
- Location: Same directory as executable
- Format: Binary (not human-readable)
- **Backup**: Copy `todo_data.dat` to preserve your data

## 📂 File Structure

```
project/
├── todo_manager_win32.c    # Main source code
├── TodoManager.exe          # Compiled executable (after build)
├── todo_data.dat            # Data file (created at runtime)
└── README.md                # This file
```

## 🐛 Troubleshooting

### Compilation Errors

**Error: "cannot find -lcomctl32"**
- **Solution**: Install MinGW-w64 (not plain MinGW)

**Error: "undefined reference to WinMain"**
- **Solution**: Add `-mwindows` flag (MinGW) or set SubSystem to Windows (MSVC)

**Error: Assembly syntax errors**
- **Solution**: The code auto-detects compiler. Ensure you're using MSVC or GCC.

### Runtime Issues

**Program crashes on startup**
- **Cause**: Corrupted `todo_data.dat`
- **Solution**: Delete `todo_data.dat` and restart

**Date validation warning**
- **Cause**: Invalid date format
- **Solution**: Use strict `YYYY-MM-DD` format (e.g., `2025-11-10`)

**Tasks not sorting**
- **Cause**: Invalid date format prevents parsing
- **Solution**: Re-enter tasks with correct date format

## 🔍 Code Architecture

### Main Components

1. **Data Layer**
   - `Task` struct: Individual task data
   - `Folder` struct: Container for multiple tasks
   - Global arrays: `folders[]`, `folder_count`, `current_folder`

2. **Business Logic**
   - `sort_tasks()`: Sorts by deadline and completion status
   - `parse_date()`: Converts string to `time_t`
   - `save_data()` / `load_data()`: Binary file I/O

3. **Assembly Layer**
   - `asm_add()`, `asm_subtract()`, `asm_increment()`
   - Compiler-agnostic inline assembly
   - Used for folder/task counting operations

4. **GUI Layer**
   - `WindowProc()`: Main event handler
   - `WM_CREATE`: Initializes all UI controls
   - `WM_COMMAND`: Handles button clicks and list selections
   - `UpdateFolderList()` / `UpdateTaskList()`: Refresh UI

### Control IDs
```c
IDC_LISTBOX_FOLDERS   1001  // Left panel (lists)
IDC_LISTBOX_TASKS     1002  // Right panel (tasks)
IDC_BTN_CREATE_LIST   1003  // Create new list
IDC_BTN_DELETE_LIST   1004  // Delete selected list
IDC_BTN_ADD_TASK      1005  // Add new task
IDC_BTN_COMPLETE_TASK 1006  // Mark task complete
IDC_BTN_DELETE_TASK   1007  // Delete selected task
IDC_BTN_SAVE          1008  // Save data manually
IDC_BTN_LOAD          1009  // Load data manually
IDC_EDIT_LIST_NAME    1010  // Text input for list name
IDC_EDIT_TASK_DESC    1011  // Text input for task description
IDC_EDIT_DEADLINE     1012  // Text input for deadline
```

## 🔧 Extending the Application

### Adding New Features

**To add a new button:**
```c
// 1. Define control ID
#define IDC_BTN_MY_FEATURE 1020

// 2. Create button in WM_CREATE
CreateWindowEx(0, "BUTTON", "My Feature",
    WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
    x, y, width, height,
    hwnd, (HMENU)IDC_BTN_MY_FEATURE, NULL, NULL);

// 3. Handle click in WM_COMMAND
case IDC_BTN_MY_FEATURE:
    MyFeatureFunction();
    break;
```

**To modify task properties:**
1. Update `Task` struct
2. Update `save_data()` / `load_data()` format
3. **Important**: Old data files won't be compatible

## 📝 Notes for Developers

### Best Practices
- Always validate user input (dates, empty fields)
- Use `MessageBox()` for user feedback
- Call `UpdateFolderList()` and `UpdateTaskList()` after data changes
- Test with maximum limits (50 tasks, 20 folders)

### Known Limitations
- No Unicode support (ASCII only)
- Fixed maximum sizes (50 tasks, 20 folders)
- No undo/redo functionality
- No task priority levels
- No recurring tasks

### Performance
- All operations are O(n) or better
- `qsort()` for task sorting: O(n log n)
- No memory allocation (all static arrays)
- Fast startup/shutdown (<100ms typical)

## 📄 License

This is educational software. Feel free to modify and distribute.

## 👥 Contributing

This project is open for educational purposes. Suggested improvements:
- Add task categories/tags
- Implement search functionality
- Add drag-and-drop reordering
- Export to CSV/JSON
- Add task notes/descriptions
- Implement due date reminders

## 📞 Support

For issues or questions:
1. Check the Troubleshooting section
2. Verify your compiler and Windows version
3. Test with a fresh `todo_data.dat` (delete old file)

---

**Version**: 1.0  
**Last Updated**: November 2025  
**Compatibility**: Windows 7/8/10/11, x86/x64