/**
 * Example: WWQueryExW
 *
 * Sends an HTTP request using the full WW_REQUESTW struct, giving control
 * over user agent, custom headers, redirect limit, and logging.
 */

#include "../../source/winweb.h"
#include <stdio.h>

int main(void)
{
    const char body[] = "{\"hello\":\"world\"}";

    WW_REQUESTW request = {
        .url              = L"https://httpbin.org/post",
        .verb             = L"POST",
        .userAgent        = L"MyApp/1.0",
        .contentType      = L"application/json",
        .body             = body,
        .bodySize         = (DWORD)(sizeof(body) - 1),
        .headers          = L"X-Custom-Header: example\r\n",
        .maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT,
        .logEnabled       = FALSE
    };

    WW_RESPONSEW response = {0};

    int result = WWQueryExW(&request, &response);

    if (result == WW_SUCCESS)
    {
        wprintf(L"Status: %lu\n", response.statusCode);
        wprintf(L"Body (%zu bytes):\n%.*S\n",
                response.dataSize, (int)response.dataSize,
                (const char*)response.data);
    }
    else
    {
        wprintf(L"Request failed (errorcode %d)\n", response.errorcode);
    }

    WWFreeResponseW(&response);

    return result;
}
