#include "file_dialogs.h"

#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <ShlObj.h>
#include <shellapi.h>

#include "core/application.h"

namespace sky
{
namespace helper
{
std::string openFile(const char *filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = {0};
    CHAR currentDir[256] = {0};
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = glfwGetWin32Window((GLFWwindow *)Application::getWindow()->getGLFWwindow());
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    if (GetCurrentDirectoryA(256, currentDir)) ofn.lpstrInitialDir = currentDir;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) return ofn.lpstrFile;

    return std::string();
}

std::string openDirectory()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return std::string();

    std::string directoryPath;

    // Create the File Open Dialog object
    IFileDialog *pFileDialog = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));

    if (SUCCEEDED(hr))
    {
        // Set the options to select folders instead of files
        DWORD dwOptions;
        if (SUCCEEDED(pFileDialog->GetOptions(&dwOptions)))
        {
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
        }

        // Show the dialog
        hr = pFileDialog->Show(NULL);
        if (SUCCEEDED(hr))
        {
            // Get the selected item
            IShellItem *pItem = nullptr;
            hr = pFileDialog->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                // Retrieve the file system path
                PWSTR pszFilePath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                // Convert to std::string if successful
                if (SUCCEEDED(hr))
                {
                    directoryPath = std::string(pszFilePath, pszFilePath + wcslen(pszFilePath));
                    CoTaskMemFree(pszFilePath); // Free the memory allocated for the path
                }

                pItem->Release();
            }
        }

        pFileDialog->Release();
    }

    CoUninitialize();
    return directoryPath;
}

std::string saveFile(const char *filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = {0};
    CHAR currentDir[256] = {0};
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = glfwGetWin32Window((GLFWwindow *)Application::getWindow()->getGLFWwindow());
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    if (GetCurrentDirectoryA(256, currentDir)) ofn.lpstrInitialDir = currentDir;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    // Sets the default extension by extracting it from the filter
    ofn.lpstrDefExt = strchr(filter, '\0') + 1;

    if (GetSaveFileNameA(&ofn) == TRUE) return ofn.lpstrFile;

    return std::string();
}

void openFolderInExplorer(const fs::path &path)
{
    ShellExecuteA(NULL, "open", path.string().c_str(), NULL, NULL, SW_SHOW);
}
} // namespace helper
} // namespace sky