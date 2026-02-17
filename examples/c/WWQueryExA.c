/**
 * Example: WWQueryExA
 *
 * Sends an HTTP request using the full WW_REQUESTA struct (ANSI version),
 * with custom headers and logging enabled.
 */

#include "../../source/winweb.h"
#include <stdio.h>

int main(void)
{
    const char body[] = "name=John&age=30";

    WW_REQUESTA request = {
        .url              = "https://httpbin.org/post",
        .verb             = "POST",
        .userAgent        = WW_DEFAULT_USER_AGENTA,
        .contentType      = "application/x-www-form-urlencoded",
        .body             = body,
        .bodySize         = (DWORD)(sizeof(body) - 1),
        .headers          = "X-Custom-Header: example\r\n",
        .maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT,
        .logEnabled       = FALSE
    };

    WW_RESPONSEA response = {0};

    int result = WWQueryExA(&request, &response);

    if (result == WW_SUCCESS)
    {
        printf("Status: %lu\n", response.statusCode);
        printf("Body (%zu bytes):\n%.*s\n",
               response.dataSize, (int)response.dataSize,
               (const char*)response.data);
    }
    else
    {
        printf("Request failed (errorcode %d)\n", response.errorcode);
    }

    WWFreeResponseA(&response);

    return result;
}
