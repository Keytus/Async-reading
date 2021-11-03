// Lab4.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "framework.h"
#include "Lab4.h"
#include <string>
#include <sstream>
#include <strsafe.h>
#include <chrono>
#include <vector>

#define MAX_LOADSTRING 100
#define IDM_ASYNCBUTTON 10001
#define IDM_NORMALBUTTON 10002
#define CURRENT_MAX_LENGTH 1000000//количество символов в файле, которое нужно считать

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING] = L"Lab4";;       // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HANDLE hSourceFile = NULL;                      // дескриптор анализируемого источникового файла
LPCWSTR fileName = L"Test.txt";                 // название файла
static HWND hInfoEdit;
OVERLAPPED* overlaps;
HANDLE* hEvents;
int asyncCount = 30;                            //количество операций
static HWND hCountEdit;                           //поле ввода числа операций


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                AppendText(HWND hEditWnd, std::wstring str);
std::wstring        UpperOrLowerCase(std::wstring str);
void                Normal();
void                Async();
VOID WINAPI         CompletionRoutine(
                        DWORD dwErrorCode,               // код завершения 
                        DWORD dwNumberOfBytesTransfered, // количество прочитанных 
                        LPOVERLAPPED lpOverlapped);  // адрес структуры OVERLAPPED 

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDC_LAB4, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB4));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;// создаём экземпляр для обращения к членам класса WNDCLASSEX

    wcex.cbSize = sizeof(WNDCLASSEX);// размер структуры (в байтах)

    wcex.style = CS_HREDRAW | CS_VREDRAW;// стиль класса окошка
    wcex.lpfnWndProc = WndProc;// указатель на функцию обработки сообщений
    wcex.cbClsExtra = 0; // число дополнительных байтов при создании экземпляра приложения
    wcex.cbWndExtra = 0;// число дополнительных байтов в конце структуры
    wcex.hInstance = hInstance;// указатель на строку, содержащую имя меню, применяемого для класса
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB4));// декриптор пиктограммы
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);// дескриптор курсора
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);// дескриптор кисти для закраски фона окна
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LAB4);// указатель на имя меню (у нас его нет)
    wcex.lpszClassName = szWindowClass;// указатель на имя класса
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));// дескриптор маленькой пиктограммы

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd = CreateWindowW(
        szWindowClass,  // имя класса
        szTitle, // заголовок окошка
        WS_OVERLAPPEDWINDOW | WS_VSCROLL, // режимы отображения окошка
        CW_USEDEFAULT, // позиция окошка по оси х
        NULL, // позиция окошка по оси у (у нас х по умолчанию и этот параметр писать не нужно)
        CW_USEDEFAULT, // ширина окошка
        NULL, // высота окошка (раз ширина по умолчанию, то писать не нужно)
        (HWND)NULL, // дескриптор родительского окна
        NULL, // дескриптор меню
        HINSTANCE(hInst), // дескриптор экземпляра приложения
        NULL); // ничего не передаём из WndProc

    if (!hWnd)
    {
        // в случае некорректного создания окошка (неверные параметры и т.п.):
        MessageBox(NULL, L"Не получилось создать окно!", L"Ошибка", MB_OK);
        return FALSE;
    }

    HWND hAsyncButtonWnd = CreateWindow(_T("BUTTON"), _T("Async"), WS_CHILD | WS_VISIBLE,
        50, 50, 75, 20, hWnd, (HMENU)IDM_ASYNCBUTTON, hInst, NULL);
    HWND hNormalButtonWnd = CreateWindow(_T("BUTTON"), _T("Normal"), WS_CHILD | WS_VISIBLE,
        50, 100, 75, 20, hWnd, (HMENU)IDM_NORMALBUTTON, hInst, NULL);

    hInfoEdit = CreateWindow(L"edit", L"",
        ES_MULTILINE | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL, 150, 50, 300, 200,
        hWnd, 0, hInst, NULL);

    hCountEdit = CreateWindow(L"edit", L"30",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_RIGHT, 50, 150, 50, 20,
        hWnd, 0, hInst, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
        int wmId = LOWORD(wParam);
        TCHAR buffer[20];

        switch (wmId)
        {
            case IDM_ASYNCBUTTON:
                GetWindowText(hCountEdit, buffer, 20);
                asyncCount = _tstoi(buffer);
                Async();
                break;
            case IDM_NORMALBUTTON:
                Normal();
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


void AppendText(HWND hEditWnd, std::wstring str)//вывести текст в окно
{
    std::basic_string<TCHAR> converted(str.begin(), str.end());
    LPCTSTR Text = converted.c_str();
    int idx = GetWindowTextLength(hEditWnd);
    SendMessage(hEditWnd, EM_SETSEL, (WPARAM)idx, (LPARAM)idx);
    SendMessage(hEditWnd, EM_REPLACESEL, 0, (LPARAM)Text);
}

std::wstring UpperOrLowerCase(std::wstring str)//замена строчных на прописные и наоборот
{
    int size = str.length();
    for (int i=0;i<size;i++)
    {
        if (str[i]>92)
        {
            str[i] = str[i] - 32;
        }
        else
        {
            str[i] = str[i] + 32;
        }
    }
    return str;
}
void Normal()//просто за раз всё считываем
{
    char buffer[CURRENT_MAX_LENGTH+1] = { 0 };
    std::wstring result;
    DWORD nBytesRead = 0;

    auto start = std::chrono::system_clock::now();
    hSourceFile = CreateFileW(fileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hSourceFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, L"CreateFile error", L"Error", MB_OK);
    }
    if (ReadFile(hSourceFile, buffer, sizeof(buffer), &nBytesRead, NULL) == FALSE)
    {
        MessageBox(NULL, L"ReadFile error", L"Error", MB_OK);
    }
    result = UpperOrLowerCase(std::wstring(&buffer[0], &buffer[nBytesRead]));
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = end - start;
    AppendText(hInfoEdit, std::wstring(L"Normal:\r\n"));
    AppendText(hInfoEdit, std::to_wstring(diff.count()));
    AppendText(hInfoEdit, std::wstring(L"\r\n"));
    CloseHandle(hSourceFile);
}

void Async()//асинхронное чтение
{
    std::wstring result = L"";
    overlaps = new OVERLAPPED[asyncCount];
    hEvents = new HANDLE[asyncCount];
    std::vector<char*> buffers(asyncCount);
    int buffLength = CURRENT_MAX_LENGTH / asyncCount;
    for (int i = 0;i < asyncCount;i++)
    {
        overlaps[i] = { 0 };
        ZeroMemory(&overlaps[i], sizeof(OVERLAPPED));

        hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEvents[i] == NULL)
        {
            MessageBox(NULL, L"hEvent error", L"Error", MB_OK);
        }
        overlaps[i].hEvent = hEvents[i];
        overlaps[i].Offset = i * (buffLength);

        buffers[i] = new char[buffLength + 1];
        for (int j = 0;j < buffLength + 1;j++)
        {
            buffers[i][j] = 0;
        }
    }
    auto start = std::chrono::system_clock::now();
    hSourceFile = CreateFileW(fileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL);
    if (hSourceFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, L"CreateFile error", L"Error", MB_OK);
    }
    for (int i = 0;i < asyncCount;i++)
    {
        if (ReadFileEx(hSourceFile, buffers[i], buffLength, &overlaps[i], (LPOVERLAPPED_COMPLETION_ROUTINE)CompletionRoutine) == FALSE)
        {
            MessageBox(NULL, L"ReadFile error", L"Error", MB_OK);
        }
        buffers[i][buffLength] = '\0';
    }
    for (int i = 0;i < asyncCount;i++)
    {
        WaitForSingleObjectEx(overlaps[i].hEvent, 10000, TRUE);
        result += UpperOrLowerCase(std::wstring(&buffers[i][0], &buffers[i][buffLength]));
    }
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::wstring str = L"";
    str.append(std::wstring(L"Async "));
    str.append(std::to_wstring(asyncCount));
    str.append(std::wstring(L":\r\n"));
    str.append(std::to_wstring(diff.count()));
    str.append(std::wstring(L"\r\n"));
    //AppendText(hInfoEdit, std::wstring(L"Async:\r\n"));
    //AppendText(hInfoEdit, std::to_wstring(diff.count()));
    //AppendText(hInfoEdit, std::wstring(L"\r\n"));
    AppendText(hInfoEdit, str);
    CloseHandle(hSourceFile);

}

//исполняется,когда опеция выполнена и WaitForSingleObjectEx ждёт с bAlertable=TRUE
VOID WINAPI CompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    CloseHandle(lpOverlapped->hEvent);
}