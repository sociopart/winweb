/**
 * Example: WWDownloadW
 *
 * Downloads a file from a URL to a full local path (directory + filename
 * combined).  Optionally show progress bar and log via flags.
 */

#include "../../source/winweb.h"
#include <stdio.h>

int main(void)
{
    LPCWSTR url      = L"https://example.com/file.zip";
    LPCWSTR savePath = L"C:\\Downloads\\file.zip";

    int result = WWDownloadW(url, savePath, WW_SHOW_PROGRESSBAR);

    if (result == WW_SUCCESS)
        wprintf(L"Downloaded successfully to %s\n", savePath);
    else
        wprintf(L"Download failed (error %d)\n", result);

    return result;
}
