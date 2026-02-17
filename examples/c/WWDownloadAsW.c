/**
 * Example: WWDownloadAsW
 *
 * Downloads a file from a URL and saves it to a specific directory with a
 * specific filename (directory and filename are passed separately).
 */

#include "../../source/winweb.h"
#include <stdio.h>

int main(void)
{
    LPCWSTR url      = L"https://example.com/release.zip";
    LPCWSTR dstPath  = L"C:\\Downloads\\";
    LPCWSTR fileName = L"myrelease.zip";

    int result = WWDownloadAsW(url, dstPath, fileName, WW_SHOW_PROGRESSBAR);

    if (result == WW_SUCCESS)
        wprintf(L"Saved as %s%s\n", dstPath, fileName);
    else
        wprintf(L"Download failed (error %d)\n", result);

    return result;
}
