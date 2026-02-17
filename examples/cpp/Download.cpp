/**
 * Example: WinWeb::Download
 *
 * Downloads a file from a URL to a full local path (directory + filename
 * combined).  Uses UTF-8 std::string arguments.
 */

#include "../../source/winweb.hpp"
#include <cstdio>

int main()
{
    int result = WinWeb::Download(
        "https://example.com/file.zip",
        "C:\\Downloads\\file.zip",
        WW_SHOW_PROGRESSBAR
    );

    if (result == WW_SUCCESS)
        puts("Downloaded successfully.");
    else
        printf("Download failed (error %d)\n", result);

    return result;
}
