/**
 * Example: WinWeb::DownloadAs
 *
 * Downloads a file from a URL to a specific directory with a specific
 * filename (directory and filename passed separately).
 */

#include "../../source/winweb.hpp"
#include <cstdio>

int main()
{
    int result = WinWeb::DownloadAs(
        "https://example.com/release.zip",
        "C:\\Downloads\\",
        "myrelease.zip",
        WW_SHOW_PROGRESSBAR
    );

    if (result == WW_SUCCESS)
        puts("Saved as C:\\Downloads\\myrelease.zip");
    else
        printf("Download failed (error %d)\n", result);

    return result;
}
