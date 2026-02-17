/**
 * Example: WinWeb::Query
 *
 * Performs HTTP GET and POST requests using std::string arguments.
 * The returned Response owns the body data — no manual free needed.
 */

#include "../../source/winweb.hpp"
#include <cstdio>
#include <vector>
#include <cstdint>

int main()
{
    /* --- GET request --- */
    WinWeb::Response getResp = WinWeb::Query("https://httpbin.org/get", "GET");

    if (getResp.statusCode == 200)
    {
        printf("GET %d — %zu bytes\n", getResp.statusCode, getResp.data.size());
        printf("%s\n", getResp.text().c_str());
    }
    else
    {
        printf("GET failed: status %lu, errorcode %d\n",
               getResp.statusCode, getResp.errorcode);
    }

    /* --- POST request with a JSON body --- */
    std::vector<uint8_t> body = {
        '{', '"', 'k', 'e', 'y', '"', ':', '"', 'v', 'a', 'l', 'u', 'e', '"', '}'
    };

    WinWeb::Response postResp = WinWeb::Query(
        "https://httpbin.org/post", "POST",
        body, "application/json"
    );

    if (postResp.statusCode == 200)
    {
        printf("POST %d — %zu bytes\n", postResp.statusCode, postResp.data.size());
        printf("%s\n", postResp.text().c_str());
    }
    else
    {
        printf("POST failed: status %lu, errorcode %d\n",
               postResp.statusCode, postResp.errorcode);
    }

    return (getResp.statusCode == 200 && postResp.statusCode == 200) ? 0 : 1;
}
