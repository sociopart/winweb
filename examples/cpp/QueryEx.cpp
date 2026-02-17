/**
 * Example: WinWeb::QueryEx
 *
 * Sends an HTTP request with full control via WW_REQUESTW — custom user
 * agent, extra headers, redirect limit, and logging.  The raw WW_RESPONSEW
 * must be freed manually with WWFreeResponseW.
 */

#include "../../source/winweb.hpp"
#include <cstdio>

int main()
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

    WW_RESPONSEW response = {};

    int result = WinWeb::QueryEx(request, response);

    if (result == WW_SUCCESS)
    {
        printf("Status: %lu\n", response.statusCode);
        printf("Body (%zu bytes):\n%.*s\n",
               response.dataSize, (int)response.dataSize,
               reinterpret_cast<const char*>(response.data));
    }
    else
    {
        printf("Request failed (errorcode %d)\n", response.errorcode);
    }

    WWFreeResponseW(&response);

    return result;
}
