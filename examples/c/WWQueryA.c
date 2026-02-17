/**
 * Example: WWQueryA
 *
 * Performs HTTP GET and POST requests using the simple ANSI query API.
 * Always free the response with WWFreeResponseA when done.
 */

#include "../../source/winweb.h"
#include <stdio.h>

int main(void)
{
    /* --- GET request --- */
    WW_RESPONSEA getResp = {0};

    int result = WWQueryA("https://httpbin.org/get", "GET",
                          NULL, 0, NULL, &getResp);

    if (result == WW_SUCCESS)
    {
        printf("GET status: %lu\n", getResp.statusCode);
        printf("Body (%zu bytes):\n%.*s\n",
               getResp.dataSize, (int)getResp.dataSize,
               (const char*)getResp.data);
    }
    else
    {
        printf("GET failed (errorcode %d)\n", getResp.errorcode);
    }

    WWFreeResponseA(&getResp);

    /* --- POST request (JSON body) --- */
    const char body[]    = "{\"key\":\"value\"}";
    WW_RESPONSEA postResp = {0};

    result = WWQueryA("https://httpbin.org/post", "POST",
                      body, (DWORD)(sizeof(body) - 1),
                      "application/json", &postResp);

    if (result == WW_SUCCESS)
    {
        printf("POST status: %lu\n", postResp.statusCode);
        printf("Body (%zu bytes):\n%.*s\n",
               postResp.dataSize, (int)postResp.dataSize,
               (const char*)postResp.data);
    }
    else
    {
        printf("POST failed (errorcode %d)\n", postResp.errorcode);
    }

    WWFreeResponseA(&postResp);

    return result;
}
