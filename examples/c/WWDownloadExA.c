/**
 * Example: WWDownloadExA
 *
 * Downloads a file using the extended WW_PARAMSA struct (ANSI version).
 */

#include "../../source/winweb.h"
#include <stdio.h>

int main(void)
{
    DWORD pbFlags = WW_PB_PROGRESSBAR | WW_PB_PERCENTAGE | WW_PB_SPEED |
                    WW_PB_FILESIZE | WW_PB_ETA | WW_PB_FILENAME;

    WW_PARAMSA params = {
        .status             = WW_STATUS_INIT,
        .errorcode          = WW_ERR_NOERROR,
        .url                = "https://example.com/file.zip",
        .dstPath            = "C:\\Downloads\\",
        .outFileName        = "file.zip",
        .userAgent          = WW_DEFAULT_USER_AGENTA,
        .maxRedirectLimit   = WW_DEFAULT_REDIRECT_LIMIT,
        .headerLength       = WW_DEFAULT_HEADER_LENGTH,
        .logEnabled         = FALSE,
        .progressBarEnabled = TRUE,
        .forceDownload      = FALSE,
        .progressBarFlags   = pbFlags,
        .progressBarData    = {0},
        .progressCallback   = NULL,
        .pCallbackData      = NULL
    };

    int result = WWDownloadExA(&params);

    if (result == WW_SUCCESS)
        printf("Download complete.\n");
    else
        printf("Download failed (errorcode %d)\n", params.errorcode);

    return result;
}
