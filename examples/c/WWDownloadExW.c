/**
 * Example: WWDownloadExW
 *
 * Downloads a file using the extended WW_PARAMSW struct, giving full control
 * over user agent, redirect limit, progress callback, and more.
 */

#include "../../source/winweb.h"
#include <stdio.h>

static void progressCallback(const WWPBARINFO* info, LPVOID userData)
{
    (void)userData;
    if (info->szTotalInBytes > 0)
    {
        int pct = (int)((info->szDownloadedInBytes * 100) / info->szTotalInBytes);
        wprintf(L"\rProgress: %d%%   ", pct);
    }
}

int main(void)
{
    DWORD pbFlags = WW_PB_PROGRESSBAR | WW_PB_PERCENTAGE | WW_PB_SPEED |
                    WW_PB_FILESIZE | WW_PB_ETA | WW_PB_FILENAME;

    WW_PARAMSW params = {
        .status             = WW_STATUS_INIT,
        .errorcode          = WW_ERR_NOERROR,
        .url                = L"https://example.com/large.iso",
        .dstPath            = L"C:\\Downloads\\",
        .outFileName        = L"large.iso",
        .userAgent          = L"MyApp/1.0",
        .maxRedirectLimit   = 5,
        .headerLength       = WW_DEFAULT_HEADER_LENGTH,
        .logEnabled         = TRUE,
        .progressBarEnabled = TRUE,
        .forceDownload      = FALSE,
        .progressBarFlags   = pbFlags,
        .progressBarData    = {0},
        .progressCallback   = progressCallback,
        .pCallbackData      = NULL
    };

    int result = WWDownloadExW(&params);

    wprintf(L"\n");
    if (result == WW_SUCCESS)
        wprintf(L"Download complete.\n");
    else
        wprintf(L"Download failed (errorcode %d)\n", params.errorcode);

    return result;
}
