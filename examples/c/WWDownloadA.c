/**
 * Example: WWDownloadA
 *
 * Downloads a file from a URL using narrow (ANSI) strings.
 * Directory path and output filename are passed separately.
 */

#include "../../source/winweb.h"
#include <stdio.h>

int main(void)
{
    LPCSTR url      = "https://example.com/file.zip";
    LPCSTR dstPath  = "C:\\Downloads\\";
    LPCSTR fileName = "file.zip";

    int result = WWDownloadA(url, dstPath, fileName, WW_SHOW_PROGRESSBAR);

    if (result == WW_SUCCESS)
        printf("Downloaded successfully to %s%s\n", dstPath, fileName);
    else
        printf("Download failed (error %d)\n", result);

    return result;
}
