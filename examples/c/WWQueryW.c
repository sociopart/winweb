/**
 * Example: WWQueryW
 *
 * Performs HTTP GET and POST requests using the simple Unicode query API.
 * Response body is printed as text.  Always free the response with
 * WWFreeResponseW when done.
 */

#include "../../source/winweb.h"
#include <stdio.h>

int main(void)
{
    /* --- GET request --- */
    WW_RESPONSEW getResp = {0};

    int result = WWQueryW(L"https://httpbin.org/get", L"GET",
                          NULL, 0, NULL, &getResp);

    if (result == WW_SUCCESS)
    {
        wprintf(L"GET status: %lu\n", getResp.statusCode);
        wprintf(L"Body (%zu bytes):\n%.*S\n",
                getResp.dataSize, (int)getResp.dataSize,
                (const char*)getResp.data);
    }
    else
    {
        wprintf(L"GET failed (errorcode %d)\n", getResp.errorcode);
    }

    WWFreeResponseW(&getResp);

    /* --- POST request (JSON body) --- */
    const char body[]   = "{\"key\":\"value\"}";
    WW_RESPONSEW postResp = {0};

    result = WWQueryW(L"https://httpbin.org/post", L"POST",
                      body, (DWORD)(sizeof(body) - 1),
                      L"application/json", &postResp);

    if (result == WW_SUCCESS)
    {
        wprintf(L"POST status: %lu\n", postResp.statusCode);
        wprintf(L"Body (%zu bytes):\n%.*S\n",
                postResp.dataSize, (int)postResp.dataSize,
                (const char*)postResp.data);
    }
    else
    {
        wprintf(L"POST failed (errorcode %d)\n", postResp.errorcode);
    }

    WWFreeResponseW(&postResp);

    return result;
}
