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

// Data structures (same as original)
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

// Assembly functions (same as original)
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

// Date parsing and sorting (same as original)
time_t parse_date(const char *date_str) {
    struct tm tm = {0};
    if (strlen(date_str) == 10 && 
        sscanf(date_str, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        return mktime(&tm);
    }
    return 0;
}

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

// File I/O functions (same as original)
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
    MessageBox(hwndMain, "Data saved successfully!", "Save Complete", MB_OK | MB_ICONINFORMATION);
}

void load_data() {
    FILE *file = fopen("todo_data.dat", "rb");
    if (file == NULL) {
        return; // No saved data
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
}

// GUI Update functions
void UpdateFolderList() {
    SendMessage(hwndFolderList, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < folder_count; i++) {
        char display[MAX_LENGTH + 20];
        sprintf(display, "%s (%d tasks)", folders[i].name, folders[i].task_count);
        SendMessage(hwndFolderList, LB_ADDSTRING, 0, (LPARAM)display);
    }
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
    
    char label[MAX_LENGTH + 50];
    sprintf(label, "Current List: %s (%d tasks)", current->name, current->task_count);
    SetWindowText(hwndCurrentLabel, label);

    time_t now = time(NULL);
    for (int i = 0; i < current->task_count; i++) {
        char display[MAX_LENGTH + 50];
        char status = current->tasks[i].completed ? 'X' : ' ';
        char *overdue = "";
        
        if (!current->tasks[i].completed && current->tasks[i].deadline_time > 0 && 
            current->tasks[i].deadline_time < now) {
            overdue = " [OVERDUE]";
        }
        
        sprintf(display, "[%c] %s (Due: %s)%s", status, 
                current->tasks[i].description, current->tasks[i].deadline, overdue);
        SendMessage(hwndTaskList, LB_ADDSTRING, 0, (LPARAM)display);
    }
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
    folder_count = asm_increment(folder_count); // Using assembly function
    
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
    folder_count = asm_subtract(folder_count, 1); // Using assembly function
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

    if (strlen(deadline) != 10 || deadline[4] != '-' || deadline[7] != '-') {
        MessageBox(hwndMain, "Date format should be YYYY-MM-DD!", "Format Error", MB_OK | MB_ICONWARNING);
        return;
    }

    strcpy(current->tasks[current->task_count].description, desc);
    strcpy(current->tasks[current->task_count].deadline, deadline);
    current->tasks[current->task_count].completed = 0;
    current->tasks[current->task_count].deadline_time = parse_date(deadline);
    current->task_count = asm_increment(current->task_count); // Using assembly function
    
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
    current->task_count = asm_subtract(current->task_count, 1); // Using assembly function
    
    UpdateFolderList();
    UpdateTaskList();
}

// Window Procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create static label for current list
            hwndCurrentLabel = CreateWindowEx(
                0, "STATIC", "No list selected",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                10, 10, 760, 20,
                hwnd, (HMENU)IDC_STATIC_CURRENT, NULL, NULL
            );

            // Create folder listbox
            CreateWindowEx(
                0, "STATIC", "Lists:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                10, 40, 200, 20,
                hwnd, NULL, NULL, NULL
            );
            hwndFolderList = CreateWindowEx(
                WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                10, 60, 200, 200,
                hwnd, (HMENU)IDC_LISTBOX_FOLDERS, NULL, NULL
            );

            // Create list management controls
            CreateWindowEx(
                0, "STATIC", "New List Name:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                10, 270, 200, 20,
                hwnd, NULL, NULL, NULL
            );
            CreateWindowEx(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                10, 290, 200, 25,
                hwnd, (HMENU)IDC_EDIT_LIST_NAME, NULL, NULL
            );
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

            // Create task listbox
            CreateWindowEx(
                0, "STATIC", "Tasks:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                230, 40, 540, 20,
                hwnd, NULL, NULL, NULL
            );
            hwndTaskList = CreateWindowEx(
                WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                230, 60, 540, 400,
                hwnd, (HMENU)IDC_LISTBOX_TASKS, NULL, NULL
            );

            // Create task management controls
            CreateWindowEx(
                0, "STATIC", "Task Description:",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                230, 470, 540, 20,
                hwnd, NULL, NULL, NULL
            );
            CreateWindowEx(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                230, 490, 540, 25,
                hwnd, (HMENU)IDC_EDIT_TASK_DESC, NULL, NULL
            );
            CreateWindowEx(
                0, "STATIC", "Deadline (YYYY-MM-DD):",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                230, 520, 540, 20,
                hwnd, NULL, NULL, NULL
            );
            CreateWindowEx(
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                230, 540, 200, 25,
                hwnd, (HMENU)IDC_EDIT_DEADLINE, NULL, NULL
            );

            // Create task action buttons
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

            // Create save/load buttons
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

            // Load data at startup
            load_data();
            UpdateFolderList();
            UpdateTaskList();
            
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
                    load_data();
                    UpdateFolderList();
                    UpdateTaskList();
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