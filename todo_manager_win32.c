#include <windows.h>
#include <commctrl.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "comctl32.lib")

#define MAX_TASKS 50
#define MAX_LENGTH 100
#define MAX_FOLDERS 20

// Control IDs
#define IDC_LISTBOX_FOLDERS 1001
#define IDC_LISTBOX_TASKS 1002
#define IDC_BTN_CREATE_LIST 1003
#define IDC_BTN_DELETE_LIST 1004
#define IDC_BTN_ADD_TASK 1005
#define IDC_BTN_COMPLETE_TASK 1006
#define IDC_BTN_DELETE_TASK 1007
#define IDC_BTN_SAVE 1008
#define IDC_BTN_LOAD 1009
#define IDC_EDIT_LIST_NAME 1010
#define IDC_EDIT_TASK_DESC 1011
#define IDC_EDIT_DEADLINE 1012
#define IDC_STATIC_CURRENT 1013

// Data structures
typedef struct {
    char description[MAX_LENGTH];
    char deadline[20];
    int completed;
    time_t deadline_time;
} Task;

typedef struct {
    char name[MAX_LENGTH];
    Task tasks[MAX_TASKS];
    int task_count;
} Folder;

Folder folders[MAX_FOLDERS];
int folder_count = 0;
int current_folder = -1;

// Global window handles
HWND hwndMain;
HWND hwndFolderList;
HWND hwndTaskList;
HWND hwndCurrentLabel;

// Assembly functions
int asm_add(int a, int b) {
    int result;
#if defined(_MSC_VER)
    __asm {
        mov eax, a
        add eax, b
        mov result, eax
    }
#elif defined(__GNUC__)
    __asm__ (
        "movl %1, %%eax\n\t"
        "addl %2, %%eax\n\t"
        "movl %%eax, %0\n\t"
        : "=r" (result)
        : "r" (a), "r" (b)
        : "%eax"
    );
#else
    result = a + b;
#endif
    return result;
}

int asm_subtract(int a, int b) {
    int result;
#if defined(_MSC_VER)
    __asm {
        mov eax, a
        sub eax, b
        mov result, eax
    }
#elif defined(__GNUC__)
    __asm__ (
        "movl %1, %%eax\n\t"
        "subl %2, %%eax\n\t"
        "movl %%eax, %0\n\t"
        : "=r" (result)
        : "r" (a), "r" (b)
        : "%eax"
    );
#else
    result = a - b;
#endif
    return result;
}

int asm_increment(int a) {
    int result;
#if defined(_MSC_VER)
    __asm {
        mov eax, a
        inc eax
        mov result, eax
    }
#elif defined(__GNUC__)
    __asm__ (
        "movl %1, %%eax\n\t"
        "incl %%eax\n\t"
        "movl %%eax, %0\n\t"
        : "=r" (result)
        : "r" (a)
        : "%eax"
    );
#else
    result = a + 1;
#endif
    return result;
}

// Simplified date validation function
int validate_date(const char *date_str, int *year, int *month, int *day) {
    // Check format first
    if (strlen(date_str) != 10 || date_str[4] != '-' || date_str[7] != '-') {
        return 0;
    }
    
    // Parse the date
    if (sscanf(date_str, "%d-%d-%d", year, month, day) != 3) {
        return 0;
    }
    
    // Validate year (must be 4 digits)
    if (*year < 1000 || *year > 9999) {
        return 0;
    }
    
    // Validate month (1-12)
    if (*month < 1 || *month > 12) {
        return 0;
    }
    
    // Validate day (1-31)
    if (*day < 1 || *day > 31) {
        return 0;
    }
    
    return 1; // Valid date
}

// Parse date string to time_t
time_t parse_date(const char *date_str) {
    struct tm tm = {0};
    int year, month, day;
    
    if (validate_date(date_str, &year, &month, &day)) {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = 0;   // Start of day for accurate date comparison
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_isdst = -1; // Let system determine DST
        return mktime(&tm);
    }
    return 0;
}

// Check if a task is overdue (strictly after deadline date)
int is_overdue(time_t deadline_time) {
    if (deadline_time == 0) return 0;
    
    time_t now = time(NULL);
    
    // Convert both times to date structures
    struct tm *now_tm = localtime(&now);
    if (!now_tm) return 0;
    
    // Store current date components
    int now_year = now_tm->tm_year;
    int now_month = now_tm->tm_mon;
    int now_day = now_tm->tm_mday;
    
    // Get deadline date components
    struct tm *deadline_tm = localtime(&deadline_time);
    if (!deadline_tm) return 0;
    
    int deadline_year = deadline_tm->tm_year;
    int deadline_month = deadline_tm->tm_mon;
    int deadline_day = deadline_tm->tm_mday;
    
    // Compare dates: current date must be STRICTLY greater than deadline
    // Year comparison
    if (now_year > deadline_year) return 1;
    if (now_year < deadline_year) return 0;
    
    // Month comparison (same year)
    if (now_month > deadline_month) return 1;
    if (now_month < deadline_month) return 0;
    
    // Day comparison (same year and month)
    if (now_day > deadline_day) return 1;
    
    return 0;  // Same day or future date - not overdue
}

// Compare function for sorting tasks
int compare_tasks(const void *a, const void *b) {
    const Task *taskA = (const Task *)a;
    const Task *taskB = (const Task *)b;
    
    if (taskA->completed && !taskB->completed) return 1;
    if (!taskA->completed && taskB->completed) return -1;
    
    if (taskA->deadline_time > taskB->deadline_time) return 1;
    if (taskA->deadline_time < taskB->deadline_time) return -1;
    
    return 0;
}

void sort_tasks(Folder *folder) {
    for (int i = 0; i < folder->task_count; i++) {
        folder->tasks[i].deadline_time = parse_date(folder->tasks[i].deadline);
    }
    qsort(folder->tasks, folder->task_count, sizeof(Task), compare_tasks);
}

// File I/O functions
void save_data() {
    FILE *file = fopen("todo_data.dat", "wb");
    if (file == NULL) {
        MessageBox(hwndMain, "Error: Could not save data to file!", "Save Error", MB_OK | MB_ICONERROR);
        return;
    }

    fwrite(&folder_count, sizeof(int), 1, file);
    for (int i = 0; i < folder_count; i++) {
        fwrite(folders[i].name, sizeof(char), MAX_LENGTH, file);
        fwrite(&folders[i].task_count, sizeof(int), 1, file);
        for (int j = 0; j < folders[i].task_count; j++) {
            fwrite(&folders[i].tasks[j], sizeof(Task), 1, file);
        }
    }
    fwrite(&current_folder, sizeof(int), 1, file);
    
    fclose(file);
    MessageBox(hwndMain, "Data saved successfully to 'todo_data.dat'!", "Save Complete", MB_OK | MB_ICONINFORMATION);
}

int load_data() {
    FILE *file = fopen("todo_data.dat", "rb");
    if (file == NULL) {
        return 0; // No saved data
    }

    fread(&folder_count, sizeof(int), 1, file);
    for (int i = 0; i < folder_count; i++) {
        fread(folders[i].name, sizeof(char), MAX_LENGTH, file);
        fread(&folders[i].task_count, sizeof(int), 1, file);
        for (int j = 0; j < folders[i].task_count; j++) {
            fread(&folders[i].tasks[j], sizeof(Task), 1, file);
        }
    }
    fread(&current_folder, sizeof(int), 1, file);
    
    fclose(file);
    return 1;
}

// GUI Update functions
void UpdateFolderList() {
    SendMessage(hwndFolderList, LB_RESETCONTENT, 0, 0);
    
    // Set horizontal extent for scrolling if text is too long
    int maxWidth = 0;
    
    for (int i = 0; i < folder_count; i++) {
        char display[MAX_LENGTH + 20];
        sprintf(display, "%s (%d tasks)", folders[i].name, folders[i].task_count);
        SendMessage(hwndFolderList, LB_ADDSTRING, 0, (LPARAM)display);
        
        // Calculate text width for horizontal scrolling
        HDC hdc = GetDC(hwndFolderList);
        SIZE size;
        GetTextExtentPoint32(hdc, display, strlen(display), &size);
        if (size.cx > maxWidth) maxWidth = size.cx;
        ReleaseDC(hwndFolderList, hdc);
    }
    
    // Set horizontal extent (add padding)
    SendMessage(hwndFolderList, LB_SETHORIZONTALEXTENT, maxWidth + 20, 0);
    
    if (current_folder >= 0 && current_folder < folder_count) {
        SendMessage(hwndFolderList, LB_SETCURSEL, current_folder, 0);
    }
}

void UpdateTaskList() {
    SendMessage(hwndTaskList, LB_RESETCONTENT, 0, 0);
    
    if (current_folder == -1 || current_folder >= folder_count) {
        SetWindowText(hwndCurrentLabel, "No list selected");
        return;
    }

    Folder *current = &folders[current_folder];
    sort_tasks(current);
    
    // Update current list label with truncation if too long
    char label[MAX_LENGTH + 50];
    if (strlen(current->name) > 50) {
        char truncated[54];
        strncpy(truncated, current->name, 50);
        truncated[50] = '\0';
        sprintf(label, "Current List: %s... (%d tasks)", truncated, current->task_count);
    } else {
        sprintf(label, "Current List: %s (%d tasks)", current->name, current->task_count);
    }
    SetWindowText(hwndCurrentLabel, label);

    // Set horizontal extent for scrolling
    int maxWidth = 0;
    
    // Get current time once for all comparisons
    time_t now = time(NULL);
    
    for (int i = 0; i < current->task_count; i++) {
        char display[MAX_LENGTH + 50];
        char status = current->tasks[i].completed ? 'X' : ' ';
        char overdue_tag[15] = "";
        
        // Check if task is overdue (not completed and deadline has passed)
        if (!current->tasks[i].completed && current->tasks[i].deadline_time > 0) {
            if (is_overdue(current->tasks[i].deadline_time)) {
                strcpy(overdue_tag, " [OVERDUE]");
            }
        }
        
        sprintf(display, "[%c] %s (Due: %s)%s", status, 
                current->tasks[i].description, current->tasks[i].deadline, overdue_tag);
        SendMessage(hwndTaskList, LB_ADDSTRING, 0, (LPARAM)display);
        
        // Calculate text width for horizontal scrolling
        HDC hdc = GetDC(hwndTaskList);
        SIZE size;
        GetTextExtentPoint32(hdc, display, strlen(display), &size);
        if (size.cx > maxWidth) maxWidth = size.cx;
        ReleaseDC(hwndTaskList, hdc);
    }
    
    // Set horizontal extent (add padding)
    SendMessage(hwndTaskList, LB_SETHORIZONTALEXTENT, maxWidth + 20, 0);
}

void CreateNewList() {
    char name[MAX_LENGTH];
    if (GetDlgItemText(hwndMain, IDC_EDIT_LIST_NAME, name, MAX_LENGTH) == 0) {
        MessageBox(hwndMain, "Please enter a list name!", "Input Error", MB_OK | MB_ICONWARNING);
        return;
    }

    if (folder_count >= MAX_FOLDERS) {
        MessageBox(hwndMain, "Maximum number of lists reached!", "Limit Reached", MB_OK | MB_ICONWARNING);
        return;
    }

    strcpy(folders[folder_count].name, name);
    folders[folder_count].task_count = 0;
    folder_count = asm_increment(folder_count);
    
    current_folder = folder_count - 1;
    
    SetDlgItemText(hwndMain, IDC_EDIT_LIST_NAME, "");
    UpdateFolderList();
    UpdateTaskList();
    
    MessageBox(hwndMain, "List created successfully!", "Success", MB_OK | MB_ICONINFORMATION);
}

void DeleteCurrentList() {
    if (current_folder == -1) {
        MessageBox(hwndMain, "Please select a list first!", "No Selection", MB_OK | MB_ICONWARNING);
        return;
    }

    char msg[MAX_LENGTH + 50];
    sprintf(msg, "Delete list '%s'?", folders[current_folder].name);
    if (MessageBox(hwndMain, msg, "Confirm Delete", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }

    for (int i = current_folder; i < folder_count - 1; i++) {
        folders[i] = folders[i + 1];
    }
    folder_count = asm_subtract(folder_count, 1);
    current_folder = -1;
    
    UpdateFolderList();
    UpdateTaskList();
}

void AddNewTask() {
    if (current_folder == -1) {
        MessageBox(hwndMain, "Please select a list first!", "No Selection", MB_OK | MB_ICONWARNING);
        return;
    }

    Folder *current = &folders[current_folder];
    if (current->task_count >= MAX_TASKS) {
        MessageBox(hwndMain, "Task list is full!", "Limit Reached", MB_OK | MB_ICONWARNING);
        return;
    }

    char desc[MAX_LENGTH], deadline[20];
    if (GetDlgItemText(hwndMain, IDC_EDIT_TASK_DESC, desc, MAX_LENGTH) == 0) {
        MessageBox(hwndMain, "Please enter a task description!", "Input Error", MB_OK | MB_ICONWARNING);
        return;
    }
    if (GetDlgItemText(hwndMain, IDC_EDIT_DEADLINE, deadline, 20) == 0) {
        MessageBox(hwndMain, "Please enter a deadline!", "Input Error", MB_OK | MB_ICONWARNING);
        return;
    }

    // Simplified date validation
    int year, month, day;
    if (!validate_date(deadline, &year, &month, &day)) {
        char error_msg[200];
        sprintf(error_msg, 
            "Invalid date format!\n\n"
            "Requirements:\n"
            "- Format: YYYY-MM-DD\n"
            "- Year: 1000-9999 (4 digits)\n"
            "- Month: 1-12\n"
            "- Day: 1-31\n\n"
            "Example: 2025-12-31");
        MessageBox(hwndMain, error_msg, "Date Validation Error", MB_OK | MB_ICONERROR);
        return;
    }

    strcpy(current->tasks[current->task_count].description, desc);
    strcpy(current->tasks[current->task_count].deadline, deadline);
    current->tasks[current->task_count].completed = 0;
    current->tasks[current->task_count].deadline_time = parse_date(deadline);
    current->task_count = asm_increment(current->task_count);
    
    sort_tasks(current);
    
    SetDlgItemText(hwndMain, IDC_EDIT_TASK_DESC, "");
    SetDlgItemText(hwndMain, IDC_EDIT_DEADLINE, "");
    UpdateFolderList();
    UpdateTaskList();
    
    MessageBox(hwndMain, "Task added successfully!", "Success", MB_OK | MB_ICONINFORMATION);
}

void CompleteSelectedTask() {
    if (current_folder == -1) {
        MessageBox(hwndMain, "Please select a list first!", "No Selection", MB_OK | MB_ICONWARNING);
        return;
    }

    int sel = SendMessage(hwndTaskList, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) {
        MessageBox(hwndMain, "Please select a task!", "No Selection", MB_OK | MB_ICONWARNING);
        return;
    }

    folders[current_folder].tasks[sel].completed = 1;
    sort_tasks(&folders[current_folder]);
    UpdateFolderList();
    UpdateTaskList();
    
    MessageBox(hwndMain, "Task marked as complete!", "Success", MB_OK | MB_ICONINFORMATION);
}

void DeleteSelectedTask() {
    if (current_folder == -1) {
        MessageBox(hwndMain, "Please select a list first!", "No Selection", MB_OK | MB_ICONWARNING);
        return;
    }

    int sel = SendMessage(hwndTaskList, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) {
        MessageBox(hwndMain, "Please select a task!", "No Selection", MB_OK | MB_ICONWARNING);
        return;
    }

    Folder *current = &folders[current_folder];
    
    for (int i = sel; i < current->task_count - 1; i++) {
        current->tasks[i] = current->tasks[i + 1];
    }
    current->task_count = asm_subtract(current->task_count, 1);
    
    UpdateFolderList();
    UpdateTaskList();
}

void LoadDataWithWarning() {
    // Show warning dialog
    int result = MessageBox(hwndMain,
        "WARNING: Loading saved data will OVERWRITE all current unsaved lists and tasks!\n\n"
        "Are you sure you want to continue?\n\n"
        "Click 'Yes' to load saved data (current data will be lost)\n"
        "Click 'No' to keep current data",
        "Load Data Warning",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    
    if (result != IDYES) {
        return; // User cancelled
    }
    
    // Proceed with loading
    if (load_data()) {
        UpdateFolderList();
        UpdateTaskList();
        MessageBox(hwndMain, "Data loaded successfully from 'todo_data.dat'!", "Load Complete", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBox(hwndMain, "No saved data file found.", "Load Data", MB_OK | MB_ICONINFORMATION);
    }
}

// Handle window resizing
void ResizeControls(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // Minimum size constraints
    if (width < 600) width = 600;
    if (height < 500) height = 500;
    
    // Calculate dynamic positions and sizes
    int leftPanelWidth = width / 4;
    if (leftPanelWidth < 200) leftPanelWidth = 200;
    if (leftPanelWidth > 300) leftPanelWidth = 300;
    
    int rightPanelX = leftPanelWidth + 20;
    int rightPanelWidth = width - rightPanelX - 10;
    
    // Get all control handles
    HWND hwndLabelLists = GetDlgItem(hwnd, 2001);
    HWND hwndLabelNewList = GetDlgItem(hwnd, 2002);
    HWND hwndEditListName = GetDlgItem(hwnd, IDC_EDIT_LIST_NAME);
    HWND hwndBtnCreate = GetDlgItem(hwnd, IDC_BTN_CREATE_LIST);
    HWND hwndBtnDeleteList = GetDlgItem(hwnd, IDC_BTN_DELETE_LIST);
    HWND hwndBtnSave = GetDlgItem(hwnd, IDC_BTN_SAVE);
    HWND hwndBtnLoad = GetDlgItem(hwnd, IDC_BTN_LOAD);
    
    HWND hwndLabelTasks = GetDlgItem(hwnd, 2003);
    HWND hwndLabelTaskDesc = GetDlgItem(hwnd, 2004);
    HWND hwndLabelDeadline = GetDlgItem(hwnd, 2005);
    HWND hwndEditTaskDesc = GetDlgItem(hwnd, IDC_EDIT_TASK_DESC);
    HWND hwndEditDeadline = GetDlgItem(hwnd, IDC_EDIT_DEADLINE);
    HWND hwndBtnAddTask = GetDlgItem(hwnd, IDC_BTN_ADD_TASK);
    HWND hwndBtnComplete = GetDlgItem(hwnd, IDC_BTN_COMPLETE_TASK);
    HWND hwndBtnDeleteTask = GetDlgItem(hwnd, IDC_BTN_DELETE_TASK);
    
    // === TOP SECTION ===
    // Current list label at top (with text wrapping)
    SetWindowPos(hwndCurrentLabel, NULL, 10, 10, width - 20, 20, SWP_NOZORDER);
    
    // === LEFT PANEL (Lists) ===
    int leftY = 40;
    
    // "Lists:" label
    SetWindowPos(hwndLabelLists, NULL, 10, leftY, leftPanelWidth, 18, SWP_NOZORDER);
    leftY += 20;
    
    // Lists listbox
    int listBoxHeight = height - 320;
    SetWindowPos(hwndFolderList, NULL, 10, leftY, leftPanelWidth, listBoxHeight, SWP_NOZORDER);
    leftY += listBoxHeight + 10;
    
    // "New List Name:" label
    SetWindowPos(hwndLabelNewList, NULL, 10, leftY, leftPanelWidth, 18, SWP_NOZORDER);
    leftY += 20;
    
    // New list name input
    SetWindowPos(hwndEditListName, NULL, 10, leftY, leftPanelWidth, 25, SWP_NOZORDER);
    leftY += 30;
    
    // Create/Delete buttons
    SetWindowPos(hwndBtnCreate, NULL, 10, leftY, leftPanelWidth / 2 - 5, 30, SWP_NOZORDER);
    SetWindowPos(hwndBtnDeleteList, NULL, leftPanelWidth / 2 + 15, leftY, leftPanelWidth / 2 - 5, 30, SWP_NOZORDER);
    leftY += 35;
    
    // Save/Load buttons
    SetWindowPos(hwndBtnSave, NULL, 10, leftY, leftPanelWidth / 2 - 5, 30, SWP_NOZORDER);
    SetWindowPos(hwndBtnLoad, NULL, leftPanelWidth / 2 + 15, leftY, leftPanelWidth / 2 - 5, 30, SWP_NOZORDER);
    
    // === RIGHT PANEL (Tasks) ===
    int rightY = 40;
    
    // "Tasks:" label
    SetWindowPos(hwndLabelTasks, NULL, rightPanelX, rightY, rightPanelWidth, 18, SWP_NOZORDER);
    rightY += 20;
    
    // Tasks listbox
    int taskBoxHeight = height - 290;
    SetWindowPos(hwndTaskList, NULL, rightPanelX, rightY, rightPanelWidth, taskBoxHeight, SWP_NOZORDER);
    rightY += taskBoxHeight + 10;
    
    // "Task Description:" label
    SetWindowPos(hwndLabelTaskDesc, NULL, rightPanelX, rightY, rightPanelWidth, 18, SWP_NOZORDER);
    rightY += 20;
    
    // Task description input
    SetWindowPos(hwndEditTaskDesc, NULL, rightPanelX, rightY, rightPanelWidth, 25, SWP_NOZORDER);
    rightY += 30;
    
    // "Deadline (YYYY-MM-DD):" label
    SetWindowPos(hwndLabelDeadline, NULL, rightPanelX, rightY, rightPanelWidth, 18, SWP_NOZORDER);
    rightY += 20;
    
    // Deadline input - full width like task description
    SetWindowPos(hwndEditDeadline, NULL, rightPanelX, rightY, rightPanelWidth, 25, SWP_NOZORDER);
    rightY += 30;
    
    // Task action buttons
    int buttonWidth = (rightPanelWidth - 20) / 3;
    SetWindowPos(hwndBtnAddTask, NULL, rightPanelX, rightY, buttonWidth, 30, SWP_NOZORDER);
    SetWindowPos(hwndBtnComplete, NULL, rightPanelX + buttonWidth + 10, rightY, buttonWidth, 30, SWP_NOZORDER);
    SetWindowPos(hwndBtnDeleteTask, NULL, rightPanelX + (buttonWidth + 10) * 2, rightY, buttonWidth, 30, SWP_NOZORDER);
}

// Window Procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create current label with word ellipsis style
            hwndCurrentLabel = CreateWindowEx(
                0, "STATIC", "No list selected",
                WS_VISIBLE | WS_CHILD | SS_LEFT | SS_ENDELLIPSIS,
                10, 10, 760, 20,
                hwnd, (HMENU)IDC_STATIC_CURRENT, NULL, NULL
            );

            // === LEFT PANEL ===
            // "Lists:" label
            CreateWindowEx(
                0, "STATIC", "Lists:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                10, 40, 200, 18,
                hwnd, (HMENU)2001, NULL, NULL
            );
            
            // Folder listbox with horizontal scroll
            hwndFolderList = CreateWindowEx(
                WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY,
                10, 60, 200, 200,
                hwnd, (HMENU)IDC_LISTBOX_FOLDERS, NULL, NULL
            );

            // "New List Name:" label
            CreateWindowEx(
                0, "STATIC", "New List Name:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                10, 270, 200, 18,
                hwnd, (HMENU)2002, NULL, NULL
            );
            
            // New list name input
            CreateWindowEx(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                10, 290, 200, 25,
                hwnd, (HMENU)IDC_EDIT_LIST_NAME, NULL, NULL
            );
            
            // Create/Delete buttons
            CreateWindowEx(
                0, "BUTTON", "Create List",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 320, 95, 30,
                hwnd, (HMENU)IDC_BTN_CREATE_LIST, NULL, NULL
            );
            CreateWindowEx(
                0, "BUTTON", "Delete List",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                115, 320, 95, 30,
                hwnd, (HMENU)IDC_BTN_DELETE_LIST, NULL, NULL
            );
            
            // Save/Load buttons
            CreateWindowEx(
                0, "BUTTON", "Save Data",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 360, 95, 30,
                hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL
            );
            CreateWindowEx(
                0, "BUTTON", "Load Data",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                115, 360, 95, 30,
                hwnd, (HMENU)IDC_BTN_LOAD, NULL, NULL
            );

            // === RIGHT PANEL ===
            // "Tasks:" label
            CreateWindowEx(
                0, "STATIC", "Tasks:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                230, 40, 540, 18,
                hwnd, (HMENU)2003, NULL, NULL
            );
            
            // Task listbox with horizontal scroll
            hwndTaskList = CreateWindowEx(
                WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY,
                230, 60, 540, 400,
                hwnd, (HMENU)IDC_LISTBOX_TASKS, NULL, NULL
            );

            // "Task Description:" label
            CreateWindowEx(
                0, "STATIC", "Task Description:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                230, 470, 540, 18,
                hwnd, (HMENU)2004, NULL, NULL
            );
            
            // Task description input
            CreateWindowEx(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                230, 490, 540, 25,
                hwnd, (HMENU)IDC_EDIT_TASK_DESC, NULL, NULL
            );
            
            // "Deadline (YYYY-MM-DD):" label
            CreateWindowEx(
                0, "STATIC", "Deadline (YYYY-MM-DD):",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                230, 520, 540, 18,
                hwnd, (HMENU)2005, NULL, NULL
            );
            
            // Deadline input - full width
            CreateWindowEx(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                230, 540, 540, 25,
                hwnd, (HMENU)IDC_EDIT_DEADLINE, NULL, NULL
            );

            // Task action buttons
            CreateWindowEx(
                0, "BUTTON", "Add Task",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                230, 575, 120, 30,
                hwnd, (HMENU)IDC_BTN_ADD_TASK, NULL, NULL
            );
            CreateWindowEx(
                0, "BUTTON", "Complete Task",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                360, 575, 120, 30,
                hwnd, (HMENU)IDC_BTN_COMPLETE_TASK, NULL, NULL
            );
            CreateWindowEx(
                0, "BUTTON", "Delete Task",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                490, 575, 120, 30,
                hwnd, (HMENU)IDC_BTN_DELETE_TASK, NULL, NULL
            );

            // Load data at startup
            load_data();
            UpdateFolderList();
            UpdateTaskList();
            
            break;
        }

        case WM_SIZE: {
            ResizeControls(hwnd);
            break;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 600;
            mmi->ptMinTrackSize.y = 500;
            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_BTN_CREATE_LIST:
                    CreateNewList();
                    break;
                case IDC_BTN_DELETE_LIST:
                    DeleteCurrentList();
                    break;
                case IDC_BTN_ADD_TASK:
                    AddNewTask();
                    break;
                case IDC_BTN_COMPLETE_TASK:
                    CompleteSelectedTask();
                    break;
                case IDC_BTN_DELETE_TASK:
                    DeleteSelectedTask();
                    break;
                case IDC_BTN_SAVE:
                    save_data();
                    break;
                case IDC_BTN_LOAD:
                    LoadDataWithWarning();
                    break;
                case IDC_LISTBOX_FOLDERS:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        current_folder = SendMessage(hwndFolderList, LB_GETCURSEL, 0, 0);
                        UpdateTaskList();
                    }
                    break;
            }
            break;
        }

        case WM_DESTROY:
            save_data(); // Auto-save on exit
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "TodoManagerWindowClass";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    hwndMain = CreateWindowEx(
        0,
        CLASS_NAME,
        "To-Do List Manager (C + Assembly)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 660,
        NULL, NULL, hInstance, NULL
    );

    if (hwndMain == NULL) {
        return 0;
    }

    ShowWindow(hwndMain, nCmdShow);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}