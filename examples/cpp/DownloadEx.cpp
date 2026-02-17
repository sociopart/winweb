/**
 * Example: WinWeb::DownloadEx
 *
 * Downloads a file with full control via WW_PARAMSW, including a custom
 * progress callback.  Note: WW_PARAMSW uses wide string members directly.
 */

#include "../../source/winweb.hpp"
#include <cstdio>

static void onProgress(const WWPBARINFO* info, LPVOID /*userData*/)
{
    if (info->szTotalInBytes > 0)
    {
        int pct = (int)((info->szDownloadedInBytes * 100) / info->szTotalInBytes);
        printf("\rProgress: %d%%   ", pct);
        fflush(stdout);
    }
}

int main()
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
        .progressCallback   = onProgress,
        .pCallbackData      = nullptr
    };

    int result = WinWeb::DownloadEx(params);

    puts("");
    if (result == WW_SUCCESS)
        puts("Download complete.");
    else
        printf("Download failed (errorcode %d)\n", params.errorcode);

    return result;
}
