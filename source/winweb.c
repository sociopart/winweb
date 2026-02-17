/**
 * @file winweb.c
 * @authors SASAKI Nobuyuki, Ivan Korolev
 * @version 0.666
 * @date 2022-2026
 * @copyright MIT License
 * @brief Source file for the WinWeb library.
 *
 *
 */

/****************************** MAIN DEFINITIONS ******************************/
#include "winweb.h"
#include <stdio.h>
#include <wchar.h>

/**
 * @brief Macro definitions.
 */
#define WW_STRUCT_NULL {0}

#ifdef WW_USE_SAFE_FUNCTIONS
#include <strsafe.h>
// Macros to replace wide string functions with corresponding WinAPI functions
#define wcsncpy(dest, src, count) StringCchCopyNW(dest, count, src, _TRUNCATE)
#define wcsncat(dest, src, count) StringCchCatNW(dest, count, src, _TRUNCATE)
#elif defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)
#endif

#define WW_COUNTOF(arr) (sizeof(arr) / sizeof((arr)[0]))
#define WW_STR_SYMSA(str) WW_COUNTOF(str) - strlen(str)
#define WW_STR_SYMSW(str) WW_COUNTOF(str) - wcslen(str)

// Macros for function visibility
#ifndef WW_PRIVATE
    #if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199409L))
        #define WW_PRIVATE static inline
    #elif defined(__cplusplus)
        #define WW_PRIVATE static inline
    #else
        #define WW_PRIVATE static
    #endif
    #else
        #define WW_PRIVATE extern
#endif

/**
 * @brief Enumerations for different log types in WinWeb library.
 */
enum E_WW_LOG_TYPE {
    WW_LOG_MODULE = 0,       /**< Error module */
    WW_LOG_WININET,          /**< Error WinINet */
    WW_LOG_REDIRS_EXCEEDED,  /**< Error: redirects exceeded */
    WW_LOG_UNKNOWN_SCHEME,   /**< Error: unknown scheme */
    WW_LOG_HEADER,           /**< Log header */
    WW_LOG_LASTRESPONSE      /**< Log last response */
};

/**
 * @brief Structure representing size units for download process.
 */

struct {
    double size;         /**< Size of the unit */
    LPCWSTR unit;        /**< Unit string */
} sizeUnitW[] = {
    { 1024.0 * 1024.0 * 1024.0 * 1024.0, L"TiB" },
    { 1024.0 * 1024.0 * 1024.0, L"GiB" },
    { 1024.0 * 1024.0, L"MiB" },
    { 1024.0, L"KiB" },
    { 1.0, L"B" }
};

/**
 * @brief Structures and functions (ANSI version).
 */

typedef struct {
    LPSTR szHeader;
    SIZE_T headerSize;
    UINT redirectCount;
    CHAR capturedFileName[MAX_PATH];
    CHAR fullFilePath[MAX_PATH];
} WW_PRIVATEPARAMSA;


/**
 * @brief Structures and functions (Unicode version).
 */

typedef struct {
    LPWSTR szHeader;
    SIZE_T headerSize;
    UINT redirectCount;
    WCHAR capturedFileName[MAX_PATH];
    WCHAR fullFilePath[MAX_PATH];
} WW_PRIVATEPARAMSW;


WW_PRIVATE
INT
WWDownloadProcessW(WW_PARAMSW* params, WW_PRIVATEPARAMSW* privateParams);

WW_PRIVATE
INT
WWProcessHttpW(URL_COMPONENTSW* ptrUrlC, HINTERNET hConn,
               WW_PARAMSW* userParams, WW_PRIVATEPARAMSW* privateParams);

WW_PRIVATE
INT
WWProcessFtpW(URL_COMPONENTSW* ptrUrlC, HINTERNET hConn,
              WW_PARAMSW* userParams, WW_PRIVATEPARAMSW* privateParams);

WW_PRIVATE
INT
WWRetrieveDataW(HINTERNET hFile, LONGLONG size,
                CONST FILETIME* pftLastModified,
                WW_PARAMSW* userParams,
                WW_PRIVATEPARAMSW* privateParams);

WW_PRIVATE
INT
WWMakeDownloadPathW(LPCWSTR url, LPWSTR path, DWORD len);

WW_PRIVATE
VOID 
WWLogW(BOOL logEnabled, INT msgType, LPCWSTR displayStr);


WW_PRIVATE
INT
WWGetSizeUnitW(DOUBLE sizeValue);
// generic

WW_PRIVATE
BOOL
WWIsFileModified(HANDLE hFileN, LONGLONG fileSize,
                 CONST FILETIME* pftLastWriteTime);

/******************************** PUBLIC API **********************************/

INT
WWDownloadW(
    LPCWSTR url,
    LPCWSTR fullFilePath,
    DWORD flags
)
{
    DWORD defaultPbFlags = WW_PB_PROGRESSBAR | WW_PB_PERCENTAGE | WW_PB_ETA |
        WW_PB_SPEED | WW_PB_FILESIZE | WW_PB_FILENAME;


    WW_PARAMSW userParams = {
        .status = WW_STATUS_INIT,
        .errorcode = WW_ERR_NOERROR,
        .url = url,
        .dstPath = NULL,
        .outFileName = NULL,
        .userAgent = WW_DEFAULT_USER_AGENTW,
        .maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT,
        .headerLength = WW_DEFAULT_HEADER_LENGTH,
        .logEnabled = flags & WW_SHOW_LOG,
        .progressBarEnabled = flags & WW_SHOW_PROGRESSBAR,
        .forceDownload = flags & WW_FORCE_DOWNLOAD,
        .progressBarFlags = defaultPbFlags,
        .progressBarData = {
            .ulTimeElapsedInSecs = 0,
            .szDownloadedInBytes = 0,
            .szTotalInBytes = 0,
            .dETAInSecs = 0
        }
    };

    // Extract filename and crop dstPath to needed states.
    CONST WCHAR* extractedFileName = wcsrchr(fullFilePath, L'\\') + 1;
    size_t dirPathLength = extractedFileName - fullFilePath;
    WCHAR* newPath = (WCHAR*)malloc(dirPathLength + 1);
    if (newPath == NULL)
    {
        return WW_FAILURE;
    }
    wcsncpy(newPath, fullFilePath, dirPathLength + 1);
    newPath[dirPathLength] = '\0'; // Null-terminate the directory path
    userParams.dstPath = newPath;
    userParams.outFileName = extractedFileName;

    return WWDownloadExW(&userParams);
}

INT
WWDownloadAsW(
    LPCWSTR url,
    LPCWSTR dstPath,
    LPCWSTR outFileName,
    DWORD flags
)
{
    DWORD defaultPbFlags = WW_PB_PROGRESSBAR | WW_PB_PERCENTAGE | WW_PB_ETA |
                           WW_PB_SPEED | WW_PB_FILESIZE | WW_PB_FILENAME;

    WW_PARAMSW userParams = {
        .status = WW_STATUS_INIT,
        .errorcode = WW_ERR_NOERROR,
        .url = url,
        .dstPath = dstPath,
        .outFileName = outFileName,
        .userAgent = WW_DEFAULT_USER_AGENTW,
        .maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT,
        .headerLength = WW_DEFAULT_HEADER_LENGTH,
        .logEnabled = flags & WW_SHOW_LOG,
        .progressBarEnabled = flags & WW_SHOW_PROGRESSBAR,
        .forceDownload = flags & WW_FORCE_DOWNLOAD,
        .progressBarFlags = defaultPbFlags,
        .progressBarData = {
            .ulTimeElapsedInSecs = 0,
            .szDownloadedInBytes = 0,
            .szTotalInBytes = 0,
            .dETAInSecs = 0
        }
    };

    return WWDownloadExW(&userParams);
}

INT
WWQueryW(
    LPCWSTR url,
    LPCWSTR verb,
    LPCVOID body,
    DWORD bodySize,
    LPCWSTR contentType,
    WW_RESPONSEW* response
)
{
    WW_REQUESTW request = {
        .url = url,
        .verb = verb,
        .userAgent = WW_DEFAULT_USER_AGENTW,
        .contentType = contentType,
        .body = body,
        .bodySize = bodySize,
        .headers = NULL,
        .maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT,
        .logEnabled = FALSE
    };
    return WWQueryExW(&request, response);
}

INT
WWQueryExW(
    WW_REQUESTW* request,
    WW_RESPONSEW* response
)
{
    if (NULL == request || NULL == response)
    {
        return WW_FAILURE;
    }

    ZeroMemory(response, sizeof(*response));

    if (NULL == request->url)
    {
        response->errorcode = WW_ERR_NO_URL;
        return WW_FAILURE;
    }

    LPCWSTR userAgent = request->userAgent;
    if (NULL == userAgent)
    {
        userAgent = WW_DEFAULT_USER_AGENTW;
    }

    LPCWSTR verb = request->verb;
    if (NULL == verb)
    {
        verb = L"GET";
    }

    UINT maxRedirs = request->maxRedirectLimit;
    if (0 == maxRedirs)
    {
        maxRedirs = WW_DEFAULT_REDIRECT_LIMIT;
    }

    WCHAR scheme[INTERNET_MAX_SCHEME_LENGTH] = L"";
    WCHAR hostname[INTERNET_MAX_HOST_NAME_LENGTH] = L"";
    WCHAR username[INTERNET_MAX_USER_NAME_LENGTH] = L"";
    WCHAR password[INTERNET_MAX_PASSWORD_LENGTH] = L"";
    WCHAR urlpath[INTERNET_MAX_PATH_LENGTH] = L"";

    URL_COMPONENTSW urlc = {
        sizeof(urlc),
        scheme, WW_COUNTOF(scheme),
        INTERNET_SCHEME_DEFAULT,
        hostname, WW_COUNTOF(hostname),
        0,
        username, WW_COUNTOF(username),
        password, WW_COUNTOF(password),
        urlpath, WW_COUNTOF(urlpath),
        NULL, 0
    };

    if (FALSE == InternetCrackUrlW(request->url, 0, 0, &urlc))
    {
        response->errorcode = WW_ERR_URL_PARSE;
        return WW_FAILURE;
    }

    if (urlc.nScheme != INTERNET_SCHEME_HTTP &&
        urlc.nScheme != INTERNET_SCHEME_HTTPS)
    {
        response->errorcode = WW_ERR_UNKNOWN_SCHEME;
        return WW_FAILURE;
    }

    HINTERNET hInet = InternetOpenW(userAgent, INTERNET_OPEN_TYPE_PRECONFIG,
                                     NULL, NULL, 0);
    if (NULL == hInet)
    {
        response->errorcode = WW_ERR_WININET_INIT;
        return WW_FAILURE;
    }

    HINTERNET hConn = InternetConnectW(hInet, urlc.lpszHostName, urlc.nPort,
                                        urlc.lpszUserName, urlc.lpszPassword,
                                        INTERNET_SERVICE_HTTP, 0, 0);
    if (NULL == hConn)
    {
        response->errorcode = WW_ERR_INTERNET_CONN;
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                    INTERNET_FLAG_NO_UI;

    if (INTERNET_SCHEME_HTTPS == urlc.nScheme)
    {
        dwFlags |= INTERNET_FLAG_SECURE;
    }

    LPCWSTR rgpszAcceptTypes[] = { L"*/*", NULL };

    HINTERNET hReq = HttpOpenRequestW(hConn, verb, urlc.lpszUrlPath,
                                       NULL, NULL, rgpszAcceptTypes,
                                       dwFlags, 0);
    if (NULL == hReq)
    {
        response->errorcode = WW_ERR_HTTP_REQUEST;
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    // Build headers string
    WCHAR headerBuf[2048] = L"";
    if (request->contentType != NULL)
    {
        wcsncpy(headerBuf, L"Content-Type: ", WW_COUNTOF(headerBuf));
        wcsncat(headerBuf, request->contentType, WW_STR_SYMSW(headerBuf));
        wcsncat(headerBuf, L"\r\n", WW_STR_SYMSW(headerBuf));
    }
    if (request->headers != NULL)
    {
        wcsncat(headerBuf, request->headers, WW_STR_SYMSW(headerBuf));
    }

    LPCWSTR pHeaders = (wcslen(headerBuf) > 0) ? headerBuf : NULL;
    DWORD headersLen = (pHeaders != NULL) ? (DWORD)wcslen(pHeaders) : 0;

    if (FALSE == HttpSendRequestW(hReq, pHeaders, headersLen,
                                   (LPVOID)request->body, request->bodySize))
    {
        response->errorcode = WW_ERR_HTTP_REQUEST;
        WWLogW(request->logEnabled, WW_LOG_WININET, NULL);
        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    // Get status code
    DWORD dwStatusCode = 0;
    DWORD dwQueryLen = sizeof(dwStatusCode);
    if (FALSE == HttpQueryInfoW(hReq,
                                 HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                                 &dwStatusCode, &dwQueryLen, 0))
    {
        response->errorcode = WW_ERR_HTTP_QUERY_INFO;
        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    response->statusCode = dwStatusCode;

    // Handle redirects
    if (dwStatusCode == HTTP_STATUS_MOVED ||
        dwStatusCode == HTTP_STATUS_REDIRECT ||
        dwStatusCode == HTTP_STATUS_REDIRECT_METHOD ||
        dwStatusCode == HTTP_STATUS_REDIRECT_KEEP_VERB)
    {
        WCHAR redirectUrl[INTERNET_MAX_URL_LENGTH] = L"";
        DWORD redirectLen = sizeof(redirectUrl);
        if (FALSE == HttpQueryInfoW(hReq, HTTP_QUERY_LOCATION,
                                     redirectUrl, &redirectLen, 0))
        {
            response->errorcode = WW_ERR_HTTP_QUERY_INFO;
            InternetCloseHandle(hReq);
            InternetCloseHandle(hConn);
            InternetCloseHandle(hInet);
            return WW_FAILURE;
        }

        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);

        if (maxRedirs == 0)
        {
            response->errorcode = WW_ERR_REDIRS_EXCEEDED;
            return WW_FAILURE;
        }

        WW_REQUESTW redirectReq = *request;
        redirectReq.url = redirectUrl;
        redirectReq.maxRedirectLimit = maxRedirs - 1;
        return WWQueryExW(&redirectReq, response);
    }

    // Read response body
    SIZE_T bufCapacity = 0x10000; // 64 KiB initial
    SIZE_T bufUsed = 0;
    LPBYTE buf = (LPBYTE)malloc(bufCapacity);
    if (NULL == buf)
    {
        response->errorcode = WW_ERR_MALLOC;
        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    DWORD bytesRead = 0;
    while (TRUE)
    {
        if (FALSE == InternetReadFile(hReq, buf + bufUsed,
                                       (DWORD)(bufCapacity - bufUsed),
                                       &bytesRead))
        {
            free(buf);
            response->errorcode = WW_ERR_HTTP_REQUEST;
            InternetCloseHandle(hReq);
            InternetCloseHandle(hConn);
            InternetCloseHandle(hInet);
            return WW_FAILURE;
        }

        if (0 == bytesRead)
        {
            break;
        }

        bufUsed += bytesRead;

        if (bufUsed >= bufCapacity)
        {
            bufCapacity *= 2;
            LPBYTE newBuf = (LPBYTE)realloc(buf, bufCapacity);
            if (NULL == newBuf)
            {
                free(buf);
                response->errorcode = WW_ERR_MALLOC;
                InternetCloseHandle(hReq);
                InternetCloseHandle(hConn);
                InternetCloseHandle(hInet);
                return WW_FAILURE;
            }
            buf = newBuf;
        }
    }

    response->data = buf;
    response->dataSize = bufUsed;

    InternetCloseHandle(hReq);
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);

    return WW_SUCCESS;
}

VOID
WWFreeResponseW(
    WW_RESPONSEW* response
)
{
    if (NULL != response)
    {
        free(response->data);
        response->data = NULL;
        response->dataSize = 0;
    }
}

INT
WWDownloadExW(
                WW_PARAMSW* userParams
             )
{
    SIZE_T headerSizeTemp = userParams->headerLength * sizeof(LPWSTR);
    WW_PRIVATEPARAMSW privateParams = {
        .headerSize = headerSizeTemp,
        .szHeader = (LPWSTR)malloc(headerSizeTemp),
        .redirectCount = 0
    };

    // Check if memory allocation failed
    if (NULL == privateParams.szHeader) 
    {
        userParams->errorcode = WW_ERR_MALLOC;
        return WW_FAILURE;
    }

    ZeroMemory(privateParams.szHeader, userParams->headerLength);


    if (NULL == userParams->url)
    {
        userParams->status = WW_STATUS_ERROR;
        return WW_FAILURE;
    }

    if (NULL == userParams->userAgent)
    {
        userParams->userAgent = WW_DEFAULT_USER_AGENTW;
    }

    if (0 > userParams->maxRedirectLimit)
    {
        userParams->maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT;
    }

    if (0 >= userParams->headerLength)
    {
        userParams->headerLength = WW_DEFAULT_HEADER_LENGTH;
    }

    INT iStatus = WWDownloadProcessW(userParams, &privateParams);

    free(privateParams.szHeader);
    return iStatus;
}
/******************************** PRIVATE API *********************************/
/**
 * @brief Unicode API implementation.
 */

WW_PRIVATE
INT
WWDownloadProcessW(
                   WW_PARAMSW* userParams, 
                   WW_PRIVATEPARAMSW* privateParams
                  ) 
{
    if (privateParams->redirectCount > userParams->maxRedirectLimit) 
    {
        userParams->errorcode = WW_ERR_REDIRS_EXCEEDED;
        WWLogW(userParams->logEnabled, WW_LOG_REDIRS_EXCEEDED, NULL);
        return WW_FAILURE;
    }

    WCHAR scheme[INTERNET_MAX_SCHEME_LENGTH] = L"";
    WCHAR hostname[INTERNET_MAX_HOST_NAME_LENGTH] = L"";
    WCHAR username[INTERNET_MAX_USER_NAME_LENGTH] = L"";
    WCHAR password[INTERNET_MAX_PASSWORD_LENGTH] = L"";
    WCHAR urlpath[INTERNET_MAX_PATH_LENGTH] = L"";

    URL_COMPONENTSW urlc = {
        sizeof(urlc),
        scheme, WW_COUNTOF(scheme),
        INTERNET_SCHEME_DEFAULT,
        hostname, WW_COUNTOF(hostname),
        0,
        username, WW_COUNTOF(username),
        password, WW_COUNTOF(password),
        urlpath, WW_COUNTOF(urlpath),
        NULL, 0
    };

    // Crack the URL
    if (FALSE == InternetCrackUrlW(userParams->url, 0, 0, &urlc))
    {
        userParams->status = WW_ERR_URL_PARSE;
        WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
        return WW_FAILURE;
    }

    DWORD dwService = 0;
    DWORD dwFlags = 0;

    switch (urlc.nScheme)
    {
        case INTERNET_SCHEME_FTP:
            dwService = INTERNET_SERVICE_FTP;
            dwFlags = INTERNET_FLAG_PASSIVE;
            break;
        case INTERNET_SCHEME_HTTP:
        case INTERNET_SCHEME_HTTPS:
            dwService = INTERNET_SERVICE_HTTP;
            break;
        default:
            userParams->errorcode = WW_ERR_UNKNOWN_SCHEME;
            WWLogW(userParams->logEnabled, WW_LOG_UNKNOWN_SCHEME, 
                   urlc.lpszScheme);
            return WW_FAILURE;
            break;
    }

    // Initialize WinINet
    HINTERNET hInet = InternetOpenW(userParams->userAgent,
                                    INTERNET_OPEN_TYPE_PRECONFIG,
                                    NULL, NULL, 0);
    if (NULL == hInet)
    {
        userParams->errorcode = WW_ERR_WININET_INIT;
        WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
        return WW_FAILURE;
    }

    // Connect to the server
    HINTERNET hConn = InternetConnectW(hInet, urlc.lpszHostName, urlc.nPort,
                                       urlc.lpszUserName, urlc.lpszPassword,
                                       dwService, dwFlags, 0);

    if (NULL == hConn)
    {
        userParams->errorcode = WW_ERR_INTERNET_CONN;
        WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
        return WW_FAILURE;
    }

    INT iResult = WW_FAILURE;

    // Process the download based on the URL scheme
    switch (urlc.nScheme) 
    {
        case INTERNET_SCHEME_FTP:
            iResult = WWProcessFtpW(&urlc, hConn, userParams, privateParams);
            break;
        case INTERNET_SCHEME_HTTP:
        case INTERNET_SCHEME_HTTPS:
            iResult = WWProcessHttpW(&urlc, hConn, userParams, privateParams);
            break;
        default:
            break;
    }

    // Close the handles
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);
    return iResult;
}

WW_PRIVATE
INT 
WWProcessHttpW(
               URL_COMPONENTSW* ptrUrlC, 
               HINTERNET hConn,
               WW_PARAMSW* userParams,
               WW_PRIVATEPARAMSW* privateParams
              )
{
    // Check if URL_COMPONENTSW pointer is NULL
    if (NULL == ptrUrlC)
    {
        userParams->errorcode = WW_ERR_EMPTY_URLC;
        return WW_FAILURE;
    }

    DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                    INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_NO_COOKIES |
                    INTERNET_FLAG_NO_UI;

    if (INTERNET_SCHEME_HTTPS == ptrUrlC->nScheme)
    {
        dwFlags = (dwFlags | INTERNET_FLAG_SECURE);
    }

    // Set the accept types for the HTTP request
    LPCWSTR rgpszAcceptTypes[] = { L"*/*", NULL };

    // Open an HTTP request handle and send the request
    HINTERNET hReq = HttpOpenRequestW(hConn, NULL, 
                                      ptrUrlC->lpszUrlPath, NULL, NULL, 
                                      rgpszAcceptTypes, dwFlags, 0);

    if (NULL == hReq || FALSE == HttpSendRequestW(hReq, NULL, 0, NULL, 0))
    {
        userParams->errorcode = WW_ERR_HTTP_REQUEST;
        WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
        if (hReq != NULL)
        {
            InternetCloseHandle(hReq);
        }
        return WW_FAILURE;
    }

    DWORD dwQueryLength = 0;
    ZeroMemory(privateParams->szHeader, privateParams->headerSize);
    dwQueryLength = userParams->headerLength;
    if (HttpQueryInfoW(hReq,
        HTTP_QUERY_RAW_HEADERS_CRLF |
        HTTP_QUERY_FLAG_REQUEST_HEADERS,
        privateParams->szHeader, &dwQueryLength, 0) == FALSE) {
        WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
        InternetCloseHandle(hReq);
        return WW_FAILURE;
    }

    WWLogW(userParams->logEnabled, WW_LOG_HEADER, privateParams->szHeader);

    ZeroMemory(privateParams->szHeader, privateParams->headerSize);
    dwQueryLength = userParams->headerLength;
    if (HttpQueryInfoW(hReq, HTTP_QUERY_RAW_HEADERS_CRLF,
        privateParams->szHeader, &dwQueryLength, 0) == FALSE) {
        WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
        InternetCloseHandle(hReq);
        return WW_FAILURE;
    }

    WWLogW(userParams->logEnabled, WW_LOG_HEADER, privateParams->szHeader);

    DWORD dwStatusCode = 0;
    dwQueryLength = sizeof(dwStatusCode);

    if (HttpQueryInfoW(hReq, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
        &dwStatusCode, &dwQueryLength, 0) == FALSE) {
        WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
        InternetCloseHandle(hReq);
        return FALSE;
    }

    // Handle different HTTP status codes
    switch (dwStatusCode)
    {
        case HTTP_STATUS_OK:
            break;
        case HTTP_STATUS_MOVED:
        case HTTP_STATUS_REDIRECT:
        case HTTP_STATUS_REDIRECT_METHOD:
        case HTTP_STATUS_REDIRECT_KEEP_VERB:
            // Handle redirect scenarios
            ZeroMemory(privateParams->szHeader, privateParams->headerSize);
            dwQueryLength = userParams->headerLength;
            if (FALSE == HttpQueryInfoW(hReq, HTTP_QUERY_LOCATION, 
                                        privateParams->szHeader,
                                        &dwQueryLength, 0))
            {
                userParams->errorcode = WW_ERR_HTTP_QUERY_INFO;
                WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
                InternetCloseHandle(hReq);
                return WW_FAILURE;
            }
            InternetCloseHandle(hReq);
            InternetCloseHandle(hConn);
            userParams->url = privateParams->szHeader;
            return WWDownloadProcessW(userParams, privateParams);
            break;
        default:
            // Handle other HTTP status codes
            InternetCloseHandle(hReq);
            return WW_FAILURE;
            break;
    }


    // Try to get filename and path from URL contents
    if (WW_FAILURE == \
            WWMakeDownloadPathW(ptrUrlC->lpszUrlPath, 
                                privateParams->capturedFileName,
                                WW_COUNTOF(privateParams->capturedFileName)))
    {
        return WW_FAILURE;
    }
    // Try to get filename and path from content disposition header
    ZeroMemory(privateParams->szHeader, privateParams->headerSize);
    dwQueryLength = userParams->headerLength;

    if (TRUE == HttpQueryInfoW(hReq, HTTP_QUERY_CONTENT_DISPOSITION,
                               privateParams->szHeader, &dwQueryLength, 0))
    {
        LPWSTR pattachment = wcsstr(privateParams->szHeader, L"attachment;");
        if (NULL != pattachment)
        {
            LPWSTR pfilename = wcsstr(pattachment, L"filename");
            if (NULL != pfilename)
            {
                // Extract the filename from the content disposition
                size_t ipath = wcsspn(pfilename + 8, L" =\"");
                LPWSTR pcontent = pfilename + 8 + ipath;
                LPWSTR pch = pcontent;
                pch = wcspbrk(pch, L";\"");
                if (NULL != pch)
                {
                    *pch = L'\0';
                }
                pch = pcontent;
                while ((pch = wcspbrk(pch, L"\\/:*?\"<>|")) != NULL)
                {
                    *pch = L'_';
                }
                if (wcslen(pcontent) != 0)
                {
                    // Use the extracted filename as the local path
                    wcsncpy(privateParams->capturedFileName, pcontent, 
                            WW_COUNTOF(privateParams->capturedFileName));
                }
            }
        }
    }

    // Retrieve the content length
    dwQueryLength = sizeof(privateParams->szHeader);
    LONGLONG lDataLength = 0;
    DWORD bufferSize = sizeof(lDataLength);
    DWORD clFlags = HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER;
    if (FALSE == HttpQueryInfoW(hReq, clFlags, &lDataLength, &bufferSize, NULL))
    {
        return WW_FAILURE;
    }
    FILETIME ftLastModified = WW_STRUCT_NULL;
    SYSTEMTIME stLastModified = WW_STRUCT_NULL;
    dwQueryLength = sizeof(stLastModified);
    if (TRUE == HttpQueryInfoW(hReq,
                                HTTP_QUERY_LAST_MODIFIED | 
                                HTTP_QUERY_FLAG_SYSTEMTIME,
                                &stLastModified, &dwQueryLength, 0)) 
    {
        SystemTimeToFileTime(&stLastModified, &ftLastModified);
    }

    // Retrieve the data and write it to disk
    INT iStatus = WWRetrieveDataW(hReq, lDataLength, &ftLastModified, 
                                  userParams, privateParams);

    // Close the request handle
    InternetCloseHandle(hReq);

    return iStatus;
}

WW_PRIVATE
INT
WWProcessFtpW(
              URL_COMPONENTSW* ptrUrlC, 
              HINTERNET hConn,
              WW_PARAMSW* userParams, 
              WW_PRIVATEPARAMSW* privateParams
             )
{
    // Check if URL_COMPONENTSW pointer is NULL
    if (NULL == ptrUrlC)
    {
        userParams->errorcode = WW_ERR_EMPTY_URLC;
        return WW_FAILURE;
    }

    WIN32_FIND_DATAW finddata;
    HINTERNET hFind = FtpFindFirstFileW(hConn, ptrUrlC->lpszUrlPath,
                                        &finddata, 0, 0);
    if (NULL != hFind)
    {
        InternetCloseHandle(hFind);
    }
    else
    {
        return WW_FAILURE;
    }

    HINTERNET hFile = FtpOpenFileW(hConn, ptrUrlC->lpszUrlPath, GENERIC_READ,
                                   FTP_TRANSFER_TYPE_BINARY, 0);
    if (NULL == hFile)
    {
        WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
        return WW_FAILURE;
    }

    LARGE_INTEGER liFileSize = { finddata.nFileSizeLow, 0 };

    wcsncpy(privateParams->capturedFileName, finddata.cFileName,
            WW_COUNTOF(privateParams->capturedFileName));

    INT iStatus = WWRetrieveDataW(hFile, liFileSize.QuadPart,
                                  &finddata.ftLastWriteTime,
                                  userParams, privateParams);

    InternetCloseHandle(hFile);

    return iStatus;
}

WW_PRIVATE
INT
WWPrepareFilePathW(
    WW_PARAMSW* userParams,
    WW_PRIVATEPARAMSW* privateParams
)
{
    ZeroMemory(privateParams->fullFilePath, MAX_PATH);
    if (NULL != userParams->dstPath)
    {
        wcsncpy(privateParams->fullFilePath, userParams->dstPath,
            WW_STR_SYMSW(privateParams->fullFilePath));
    }
    if (NULL != userParams->outFileName)
    {
        if (NULL == privateParams->fullFilePath)
        {
            wcsncpy(privateParams->fullFilePath, userParams->outFileName,
                WW_STR_SYMSW(privateParams->fullFilePath));
        }
        else
        {
            wcsncat(privateParams->fullFilePath, userParams->outFileName,
                WW_STR_SYMSW(privateParams->fullFilePath));
        }
    }
    else
    {
        if (NULL == privateParams->capturedFileName)
        {
            return WW_FAILURE;
        }
        else
        {
            wcsncat(privateParams->fullFilePath,
                privateParams->capturedFileName,
                WW_STR_SYMSW(privateParams->fullFilePath));
        }
    }
    return WW_SUCCESS;
}

WW_PRIVATE
INT
WWRetrieveDataW(
                HINTERNET hFile, 
                LONGLONG fileSize, 
                CONST FILETIME* pftLastModified,
                WW_PARAMSW* userParams,
                WW_PRIVATEPARAMSW* privateParams
               )
{
    static BYTE bufRead[0x10000];   //64KiB
    BOOL retRead;
    DWORD bytesRead, byteWrite;
    INT ratio = 0;
    WWPBARINFO* pbar = &userParams->progressBarData;
    pbar->szTotalInBytes = fileSize;
    pbar->szDownloadedInBytes = 0;
    LONGLONG szDownloadedPrev = 0;
    FILETIME st, st0, st1;
    WCHAR speed[32] = { L'\0' };
    WCHAR filePathTemp[MAX_PATH] = L"";

    if (WW_FAILURE == WWPrepareFilePathW(userParams, privateParams))
    { 
        return WW_FAILURE;
    }

    HANDLE hFileN = CreateFileW(privateParams->fullFilePath, GENERIC_READ, 
                                0, NULL, OPEN_EXISTING, 
                                FILE_ATTRIBUTE_NORMAL, 0);
    if (FALSE == WWIsFileModified(hFileN, fileSize, pftLastModified))
    {
        CloseHandle(hFileN);
        return WW_SUCCESS;
    }
    CloseHandle(hFileN);

    wcsncpy(filePathTemp, privateParams->fullFilePath, 
            WW_STR_SYMSW(filePathTemp));
    wcsncat(filePathTemp, L"~", WW_STR_SYMSW(filePathTemp));

    HANDLE hf = CreateFileW(privateParams->fullFilePath, GENERIC_WRITE, 0, 
                            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE == hf) 
    {
        userParams->errorcode = WW_ERR_CREATE_FILE;
        WWLogW(userParams->logEnabled, WW_LOG_MODULE, NULL);
        return WW_FAILURE;
    }

    HANDLE hft = CreateFileW(filePathTemp, GENERIC_WRITE, 0, NULL, 
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE == hft)
    {
        userParams->errorcode = WW_ERR_CREATE_FILE;
        WWLogW(userParams->logEnabled, WW_LOG_MODULE, NULL);
        CloseHandle(hf);
        return WW_FAILURE;
    }

    GetSystemTimeAsFileTime(&st);
    st0 = st;
    st1 = st;

    DOUBLE averageSpeed = 0.0;
    DOUBLE remainingSize = (DOUBLE)pbar->szTotalInBytes;
    LPCWSTR fileNamePtr = NULL;
    if (userParams->progressBarFlags & WW_PB_FILENAME)
    {
        fileNamePtr = userParams->outFileName;
        if (NULL == fileNamePtr)
        {
            fileNamePtr = privateParams->capturedFileName;
        }
    }
    while (TRUE)
    {
        ZeroMemory(bufRead, sizeof(bufRead));
        retRead = InternetReadFile(hFile, bufRead, sizeof(bufRead), &bytesRead);
        if (retRead)
        {
            if (bytesRead == 0)
            {
                break;
            }
        }
        else
        {
            WWLogW(userParams->logEnabled, WW_LOG_WININET, NULL);
            CloseHandle(hft);
            CloseHandle(hf);
            return WW_FAILURE;
        }

        if ((FALSE == WriteFile(hft, bufRead, bytesRead, &byteWrite, NULL)) ||
            bytesRead != byteWrite)
        {
            WWLogW(userParams->logEnabled, WW_LOG_MODULE, NULL);
            CloseHandle(hft);
            CloseHandle(hf);
            return WW_FAILURE;
        }

        pbar->szDownloadedInBytes += bytesRead;

        if (0 != pbar->szTotalInBytes)
        {
            ratio = (INT)((pbar->szDownloadedInBytes * 10000) / pbar->szTotalInBytes);
        }

        WCHAR progressbar[64];
        ZeroMemory(progressbar, sizeof(progressbar));
        progressbar[0] = L'[';
        for (INT i = 1; i < 24; i++)
        {
            if (ratio / 350 > i)
            {
                progressbar[i] = L'#';
            }
            else
            {
                progressbar[i] = L'-';
            }
        }
        progressbar[24] = L']';
        GetSystemTimeAsFileTime(&st1);
        ULONGLONG difftime = ((PULARGE_INTEGER)&st1)->QuadPart - \
            ((PULARGE_INTEGER)&st0)->QuadPart;
        if ((difftime >= 10000000) || ((ratio != 0) && (ratio % 10000 == 0)))
        {
            DOUBLE diffsize =
                (DOUBLE)(pbar->szDownloadedInBytes - szDownloadedPrev) /
                ((DOUBLE)difftime / (DOUBLE)10000000);

            pbar->ulTimeElapsedInSecs = ((PULARGE_INTEGER)&st1)->QuadPart - \
                ((PULARGE_INTEGER)&st)->QuadPart;
            ULONGLONG difftimeS = pbar->ulTimeElapsedInSecs;
            INT diffsec = (INT)((difftimeS - (difftimeS % 10000000)) / 10000000);
            INT speedUnitIndex = WWGetSizeUnitW(diffsize);

            st0 = st1;


            INT dwnSizeUnit = WWGetSizeUnitW((DOUBLE)pbar->szDownloadedInBytes);
            INT totlSizeUnit = WWGetSizeUnitW((DOUBLE)pbar->szTotalInBytes);

            // ETA calculation
            DOUBLE downloadSpeed = diffsize / (diffsec != 0 ? diffsec : 1);
            ULONGLONG remainingSize = pbar->szTotalInBytes - pbar->szDownloadedInBytes;
            pbar->dETAInSecs = remainingSize / downloadSpeed;
            INT etaHours = (INT)(pbar->dETAInSecs / 3600);
            INT etaMinutes = (INT)((pbar->dETAInSecs - (etaHours * 3600)) / 60);
            INT etaSeconds = (INT)(pbar->dETAInSecs - (etaHours * 3600) - (etaMinutes * 60));

            if (userParams->progressCallback != NULL)
            {
                userParams->progressCallback(pbar,
                                             userParams->pCallbackData);
            }

            wprintf(L"\r");
            if (userParams->progressBarFlags & WW_PB_FILENAME)
            {
                wprintf(L"%s ", fileNamePtr);
            }
            if (userParams->progressBarFlags & WW_PB_PROGRESSBAR)
            {
                wprintf(L"%s ", progressbar);
            }
            if (userParams->progressBarFlags & WW_PB_PERCENTAGE)
            {
                wprintf(L"%3d.%02d%%; ",
                    (ratio - (ratio % 100)) / 100, (ratio % 100));
            }
            if (userParams->progressBarFlags & WW_PB_FILESIZE)
            {
                wprintf(L"%6.2f%s /%6.2f%s; ",
                    pbar->szDownloadedInBytes / sizeUnitW[dwnSizeUnit].size,
                    sizeUnitW[dwnSizeUnit].unit,
                    pbar->szTotalInBytes / sizeUnitW[totlSizeUnit].size,
                    sizeUnitW[totlSizeUnit].unit);
            }
            if (userParams->progressBarFlags & WW_PB_ELAPSEDTIME)
            {
                wprintf(L"%02d:%02d:%02d",
                    (diffsec - (diffsec % 3600)) / 3600,
                    ((diffsec % 3600) - (diffsec % 60)) / 60,
                    diffsec % 60);
            }
            if (userParams->progressBarFlags & WW_PB_SPEED)
            {
                wprintf(L"%6.2f%s/s; ",
                    diffsize / sizeUnitW[speedUnitIndex].size,
                    sizeUnitW[speedUnitIndex].unit);
            }
            if (userParams->progressBarFlags & WW_PB_ETA)
            {
                wprintf(L"ETA: %02d:%02d:%02d ",
                    etaHours, etaMinutes, etaSeconds);
            }

            szDownloadedPrev = pbar->szDownloadedInBytes;
        }
    }

    wprintf(L"\n\n");

    if (pftLastModified != NULL)
    {
        if (SetFileTime(hft, NULL, NULL, pftLastModified) == FALSE)
        {
            WWLogW(userParams->logEnabled, WW_LOG_MODULE, NULL);
            CloseHandle(hft);
            CloseHandle(hf);
            return WW_FAILURE;
        }
    }

    CloseHandle(hft);
    CloseHandle(hf);

    if (MoveFileExW(filePathTemp, privateParams->fullFilePath,
        MOVEFILE_REPLACE_EXISTING) == FALSE)
    {
        WWLogW(userParams->logEnabled, WW_LOG_MODULE, NULL);
        return WW_FAILURE;
    }

    return WW_SUCCESS;
}

WW_PRIVATE
INT 
WWMakeDownloadPathW(
                    LPCWSTR url, 
                    LPWSTR path, 
                    DWORD len
                   ) 
{
    LPCWSTR fnurl = wcsrchr(url, L'/');
    if (fnurl == NULL || wcslen(fnurl) == 1)
    {
        wprintf(L"ERROR : local path\n");
        return WW_FAILURE;
    }
    wcsncpy(path, fnurl + 1, len);
    path[len - 1] = L'\0';

    LPWSTR ppath = wcschr(path, L'?');
    if (ppath != NULL)
    {
        *ppath = L'\0';
    }

    // Preferred File Name
    LPWSTR pfname = path;
    while ((pfname = wcspbrk(pfname, L"\\/:*?\"<>|")) != NULL)
    {
        *pfname = L'_';
    }

    return WW_SUCCESS;
}

WW_PRIVATE
INT
WWGetSizeUnitW(
                DOUBLE sizeValue
              )
{
    INT sizeUnit = 0;
    for (INT i = 0; i < WW_COUNTOF(sizeUnitW); i++)
    {
        if (sizeValue >= sizeUnitW[i].size)
        {
            sizeUnit = i;
            break;
        }
    }
    return sizeUnit;
}


WW_PRIVATE
VOID 
WWLogW(
        BOOL logEnabled, 
        INT msgType, 
        LPCWSTR displayStr
      ) 
{
    if (!logEnabled) return;
    switch (msgType) 
    {
        case WW_LOG_MODULE: case WW_LOG_WININET:
        {
            DWORD error = GetLastError();
            HMODULE hModule = NULL;

            if (msgType == WW_LOG_WININET) 
            {
                hModule = GetModuleHandleW(L"wininet.dll");
                WWLogW(logEnabled, WW_LOG_LASTRESPONSE, NULL);
            }

            LPWSTR message = NULL;

            wprintf(L"ERROR : 0x%08X\n", error);

            DWORD formatMsgFlag;

            if (hModule != NULL) 
            {
                formatMsgFlag = FORMAT_MESSAGE_FROM_HMODULE;
            }
            else 
            {
                formatMsgFlag = FORMAT_MESSAGE_FROM_SYSTEM;
            }

            FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_IGNORE_INSERTS |
                formatMsgFlag,
                hModule, error, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                (LPWSTR)&message, 0, NULL
            );

            if (message != NULL) 
            {
                wprintf(L"%s\n", message);
                LocalFree(message);
            }
            break;
        }
        case WW_LOG_REDIRS_EXCEEDED:
        {
            wprintf(L"ERROR : Redirect limit exceeded\n");
            break;
        }
        case WW_LOG_UNKNOWN_SCHEME:
        {
            wprintf(L"ERROR : Unknown scheme: %s\n", displayStr);
            break;
        }
        case WW_LOG_HEADER:
        {
            wprintf(L"=== HEADER START === \n");
            wprintf(L"%s\n", displayStr);
            wprintf(L"=== HEADER END === \n");
            break;
        }
        case WW_LOG_LASTRESPONSE:
        {
            DWORD dwError = 0;
            PWSTR szBuffer = NULL;
            DWORD dwBufferSize = 0;

            if (InternetGetLastResponseInfoW(&dwError, szBuffer,
                &dwBufferSize) == FALSE) {
                DWORD error = GetLastError();
                if (error == ERROR_INSUFFICIENT_BUFFER) {
                    dwBufferSize += 1;
                    szBuffer = (PWSTR)LocalAlloc(LPTR, dwBufferSize * sizeof(WCHAR));
                    if (szBuffer != NULL) {
                        if (InternetGetLastResponseInfoW(&dwError, szBuffer,
                            &dwBufferSize) == TRUE) {
                            wprintf(L"%s\n", szBuffer);
                        }
                        LocalFree(szBuffer);
                    }
                }
            }
            break;
        }
    }
}

/**
 * @brief Generic API
 */

WW_PRIVATE
BOOL
WWIsFileModified(
                  HANDLE hFileN, 
                  LONGLONG fileSize,
                  CONST FILETIME* pftLastWriteTime
                )
{
    BOOL bStatus = (INVALID_HANDLE_VALUE == hFileN);
    if (!bStatus)
    {
        FILETIME ft;
        LARGE_INTEGER li;

        GetFileTime(hFileN, NULL, NULL, &ft);
        GetFileSizeEx(hFileN, &li);

        bStatus = !(fileSize == li.QuadPart && \
            CompareFileTime(pftLastWriteTime, &ft) <= 0);
    }
    return bStatus;
}

/****************************** ANSI API **************************************/

/**
 * @brief ANSI logging helper.
 */
WW_PRIVATE
VOID
WWLogA(
    BOOL logEnabled,
    INT msgType,
    LPCSTR displayStr
)
{
    if (!logEnabled) return;
    switch (msgType)
    {
        case WW_LOG_MODULE: case WW_LOG_WININET:
        {
            DWORD error = GetLastError();
            HMODULE hModule = NULL;

            if (msgType == WW_LOG_WININET)
            {
                hModule = GetModuleHandleA("wininet.dll");
                WWLogA(logEnabled, WW_LOG_LASTRESPONSE, NULL);
            }

            LPSTR message = NULL;

            printf("ERROR : 0x%08X\n", (unsigned int)error);

            DWORD formatMsgFlag;

            if (hModule != NULL)
            {
                formatMsgFlag = FORMAT_MESSAGE_FROM_HMODULE;
            }
            else
            {
                formatMsgFlag = FORMAT_MESSAGE_FROM_SYSTEM;
            }

            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_IGNORE_INSERTS |
                formatMsgFlag,
                hModule, error, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                (LPSTR)&message, 0, NULL
            );

            if (message != NULL)
            {
                printf("%s\n", message);
                LocalFree(message);
            }
            break;
        }
        case WW_LOG_REDIRS_EXCEEDED:
        {
            printf("ERROR : Redirect limit exceeded\n");
            break;
        }
        case WW_LOG_UNKNOWN_SCHEME:
        {
            printf("ERROR : Unknown scheme: %s\n", displayStr);
            break;
        }
        case WW_LOG_HEADER:
        {
            printf("=== HEADER START === \n");
            printf("%s\n", displayStr);
            printf("=== HEADER END === \n");
            break;
        }
        case WW_LOG_LASTRESPONSE:
        {
            DWORD dwError = 0;
            PSTR szBuffer = NULL;
            DWORD dwBufferSize = 0;

            if (InternetGetLastResponseInfoA(&dwError, szBuffer,
                &dwBufferSize) == FALSE) {
                DWORD error = GetLastError();
                if (error == ERROR_INSUFFICIENT_BUFFER) {
                    dwBufferSize += 1;
                    szBuffer = (PSTR)LocalAlloc(LPTR, dwBufferSize);
                    if (szBuffer != NULL) {
                        if (InternetGetLastResponseInfoA(&dwError, szBuffer,
                            &dwBufferSize) == TRUE) {
                            printf("%s\n", szBuffer);
                        }
                        LocalFree(szBuffer);
                    }
                }
            }
            break;
        }
    }
}

/**
 * @brief ANSI HTTP query functions.
 */

#define WW_STR_SYMSA_BUF(str) WW_COUNTOF(str) - strlen(str)

INT
WWQueryA(
    LPCSTR url,
    LPCSTR verb,
    LPCVOID body,
    DWORD bodySize,
    LPCSTR contentType,
    WW_RESPONSEA* response
)
{
    WW_REQUESTA request = {
        .url = url,
        .verb = verb,
        .userAgent = WW_DEFAULT_USER_AGENTA,
        .contentType = contentType,
        .body = body,
        .bodySize = bodySize,
        .headers = NULL,
        .maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT,
        .logEnabled = FALSE
    };
    return WWQueryExA(&request, response);
}

INT
WWQueryExA(
    WW_REQUESTA* request,
    WW_RESPONSEA* response
)
{
    if (NULL == request || NULL == response)
    {
        return WW_FAILURE;
    }

    ZeroMemory(response, sizeof(*response));

    if (NULL == request->url)
    {
        response->errorcode = WW_ERR_NO_URL;
        return WW_FAILURE;
    }

    LPCSTR userAgent = request->userAgent;
    if (NULL == userAgent)
    {
        userAgent = WW_DEFAULT_USER_AGENTA;
    }

    LPCSTR verb = request->verb;
    if (NULL == verb)
    {
        verb = "GET";
    }

    UINT maxRedirs = request->maxRedirectLimit;
    if (0 == maxRedirs)
    {
        maxRedirs = WW_DEFAULT_REDIRECT_LIMIT;
    }

    CHAR scheme[INTERNET_MAX_SCHEME_LENGTH] = "";
    CHAR hostname[INTERNET_MAX_HOST_NAME_LENGTH] = "";
    CHAR username[INTERNET_MAX_USER_NAME_LENGTH] = "";
    CHAR password[INTERNET_MAX_PASSWORD_LENGTH] = "";
    CHAR urlpath[INTERNET_MAX_PATH_LENGTH] = "";

    URL_COMPONENTSA urlc = {
        sizeof(urlc),
        scheme, WW_COUNTOF(scheme),
        INTERNET_SCHEME_DEFAULT,
        hostname, WW_COUNTOF(hostname),
        0,
        username, WW_COUNTOF(username),
        password, WW_COUNTOF(password),
        urlpath, WW_COUNTOF(urlpath),
        NULL, 0
    };

    if (FALSE == InternetCrackUrlA(request->url, 0, 0, &urlc))
    {
        response->errorcode = WW_ERR_URL_PARSE;
        return WW_FAILURE;
    }

    if (urlc.nScheme != INTERNET_SCHEME_HTTP &&
        urlc.nScheme != INTERNET_SCHEME_HTTPS)
    {
        response->errorcode = WW_ERR_UNKNOWN_SCHEME;
        return WW_FAILURE;
    }

    HINTERNET hInet = InternetOpenA(userAgent, INTERNET_OPEN_TYPE_PRECONFIG,
                                     NULL, NULL, 0);
    if (NULL == hInet)
    {
        response->errorcode = WW_ERR_WININET_INIT;
        return WW_FAILURE;
    }

    HINTERNET hConn = InternetConnectA(hInet, urlc.lpszHostName, urlc.nPort,
                                        urlc.lpszUserName, urlc.lpszPassword,
                                        INTERNET_SERVICE_HTTP, 0, 0);
    if (NULL == hConn)
    {
        response->errorcode = WW_ERR_INTERNET_CONN;
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                    INTERNET_FLAG_NO_UI;

    if (INTERNET_SCHEME_HTTPS == urlc.nScheme)
    {
        dwFlags |= INTERNET_FLAG_SECURE;
    }

    LPCSTR rgpszAcceptTypes[] = { "*/*", NULL };

    HINTERNET hReq = HttpOpenRequestA(hConn, verb, urlc.lpszUrlPath,
                                       NULL, NULL, rgpszAcceptTypes,
                                       dwFlags, 0);
    if (NULL == hReq)
    {
        response->errorcode = WW_ERR_HTTP_REQUEST;
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    // Build headers string
    CHAR headerBuf[2048] = "";
    if (request->contentType != NULL)
    {
        strncpy(headerBuf, "Content-Type: ", WW_COUNTOF(headerBuf) - 1);
        strncat(headerBuf, request->contentType, WW_STR_SYMSA_BUF(headerBuf));
        strncat(headerBuf, "\r\n", WW_STR_SYMSA_BUF(headerBuf));
    }
    if (request->headers != NULL)
    {
        strncat(headerBuf, request->headers, WW_STR_SYMSA_BUF(headerBuf));
    }

    LPCSTR pHeaders = (strlen(headerBuf) > 0) ? headerBuf : NULL;
    DWORD headersLen = (pHeaders != NULL) ? (DWORD)strlen(pHeaders) : 0;

    if (FALSE == HttpSendRequestA(hReq, pHeaders, headersLen,
                                   (LPVOID)request->body, request->bodySize))
    {
        response->errorcode = WW_ERR_HTTP_REQUEST;
        WWLogA(request->logEnabled, WW_LOG_WININET, NULL);
        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    // Get status code
    DWORD dwStatusCode = 0;
    DWORD dwQueryLen = sizeof(dwStatusCode);
    if (FALSE == HttpQueryInfoA(hReq,
                                 HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                                 &dwStatusCode, &dwQueryLen, 0))
    {
        response->errorcode = WW_ERR_HTTP_QUERY_INFO;
        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    response->statusCode = dwStatusCode;

    // Handle redirects
    if (dwStatusCode == HTTP_STATUS_MOVED ||
        dwStatusCode == HTTP_STATUS_REDIRECT ||
        dwStatusCode == HTTP_STATUS_REDIRECT_METHOD ||
        dwStatusCode == HTTP_STATUS_REDIRECT_KEEP_VERB)
    {
        CHAR redirectUrl[INTERNET_MAX_URL_LENGTH] = "";
        DWORD redirectLen = sizeof(redirectUrl);
        if (FALSE == HttpQueryInfoA(hReq, HTTP_QUERY_LOCATION,
                                     redirectUrl, &redirectLen, 0))
        {
            response->errorcode = WW_ERR_HTTP_QUERY_INFO;
            InternetCloseHandle(hReq);
            InternetCloseHandle(hConn);
            InternetCloseHandle(hInet);
            return WW_FAILURE;
        }

        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);

        if (maxRedirs == 0)
        {
            response->errorcode = WW_ERR_REDIRS_EXCEEDED;
            return WW_FAILURE;
        }

        WW_REQUESTA redirectReq = *request;
        redirectReq.url = redirectUrl;
        redirectReq.maxRedirectLimit = maxRedirs - 1;
        return WWQueryExA(&redirectReq, response);
    }

    // Read response body
    SIZE_T bufCapacity = 0x10000; // 64 KiB initial
    SIZE_T bufUsed = 0;
    LPBYTE buf = (LPBYTE)malloc(bufCapacity);
    if (NULL == buf)
    {
        response->errorcode = WW_ERR_MALLOC;
        InternetCloseHandle(hReq);
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return WW_FAILURE;
    }

    DWORD bytesRead = 0;
    while (TRUE)
    {
        if (FALSE == InternetReadFile(hReq, buf + bufUsed,
                                       (DWORD)(bufCapacity - bufUsed),
                                       &bytesRead))
        {
            free(buf);
            response->errorcode = WW_ERR_HTTP_REQUEST;
            InternetCloseHandle(hReq);
            InternetCloseHandle(hConn);
            InternetCloseHandle(hInet);
            return WW_FAILURE;
        }

        if (0 == bytesRead)
        {
            break;
        }

        bufUsed += bytesRead;

        if (bufUsed >= bufCapacity)
        {
            bufCapacity *= 2;
            LPBYTE newBuf = (LPBYTE)realloc(buf, bufCapacity);
            if (NULL == newBuf)
            {
                free(buf);
                response->errorcode = WW_ERR_MALLOC;
                InternetCloseHandle(hReq);
                InternetCloseHandle(hConn);
                InternetCloseHandle(hInet);
                return WW_FAILURE;
            }
            buf = newBuf;
        }
    }

    response->data = buf;
    response->dataSize = bufUsed;

    InternetCloseHandle(hReq);
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);

    return WW_SUCCESS;
}

VOID
WWFreeResponseA(
    WW_RESPONSEA* response
)
{
    if (NULL != response)
    {
        free(response->data);
        response->data = NULL;
        response->dataSize = 0;
    }
}

/**
 * @brief ANSI download functions.
 */

WW_PRIVATE
INT
WWDownloadProcessA(WW_PARAMSA* params, WW_PRIVATEPARAMSA* privateParams);

WW_PRIVATE
INT
WWProcessHttpA(URL_COMPONENTSA* ptrUrlC, HINTERNET hConn,
               WW_PARAMSA* userParams, WW_PRIVATEPARAMSA* privateParams);

WW_PRIVATE
INT
WWRetrieveDataA(HINTERNET hFile, LONGLONG size,
                CONST FILETIME* pftLastModified,
                WW_PARAMSA* userParams,
                WW_PRIVATEPARAMSA* privateParams);

WW_PRIVATE
INT
WWMakeDownloadPathA(LPCSTR url, LPSTR path, DWORD len);

INT
WWDownloadA(
    LPCSTR url,
    LPCSTR dstPath,
    LPCSTR outFileName,
    DWORD flags
)
{
    DWORD defaultPbFlags = WW_PB_PROGRESSBAR | WW_PB_PERCENTAGE | WW_PB_ETA |
                           WW_PB_SPEED | WW_PB_FILESIZE | WW_PB_FILENAME;

    WW_PARAMSA userParams = {
        .status = WW_STATUS_INIT,
        .errorcode = WW_ERR_NOERROR,
        .url = url,
        .dstPath = dstPath,
        .outFileName = outFileName,
        .userAgent = WW_DEFAULT_USER_AGENTA,
        .maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT,
        .headerLength = WW_DEFAULT_HEADER_LENGTH,
        .logEnabled = flags & WW_SHOW_LOG,
        .progressBarEnabled = flags & WW_SHOW_PROGRESSBAR,
        .forceDownload = flags & WW_FORCE_DOWNLOAD,
        .progressBarFlags = defaultPbFlags,
        .progressBarData = {
            .ulTimeElapsedInSecs = 0,
            .szDownloadedInBytes = 0,
            .szTotalInBytes = 0,
            .dETAInSecs = 0
        }
    };

    return WWDownloadExA(&userParams);
}

INT
WWDownloadExA(
    WW_PARAMSA* userParams
)
{
    SIZE_T headerSizeTemp = userParams->headerLength * sizeof(LPSTR);
    WW_PRIVATEPARAMSA privateParams = {
        .headerSize = headerSizeTemp,
        .szHeader = (LPSTR)malloc(headerSizeTemp),
        .redirectCount = 0
    };

    if (NULL == privateParams.szHeader)
    {
        userParams->errorcode = WW_ERR_MALLOC;
        return WW_FAILURE;
    }

    ZeroMemory(privateParams.szHeader, userParams->headerLength);

    if (NULL == userParams->url)
    {
        userParams->status = WW_STATUS_ERROR;
        return WW_FAILURE;
    }

    if (NULL == userParams->userAgent)
    {
        userParams->userAgent = WW_DEFAULT_USER_AGENTA;
    }

    if (0 > userParams->maxRedirectLimit)
    {
        userParams->maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT;
    }

    if (0 >= userParams->headerLength)
    {
        userParams->headerLength = WW_DEFAULT_HEADER_LENGTH;
    }

    INT iStatus = WWDownloadProcessA(userParams, &privateParams);

    free(privateParams.szHeader);
    return iStatus;
}

WW_PRIVATE
INT
WWDownloadProcessA(
    WW_PARAMSA* userParams,
    WW_PRIVATEPARAMSA* privateParams
)
{
    if (privateParams->redirectCount > userParams->maxRedirectLimit)
    {
        userParams->errorcode = WW_ERR_REDIRS_EXCEEDED;
        WWLogA(userParams->logEnabled, WW_LOG_REDIRS_EXCEEDED, NULL);
        return WW_FAILURE;
    }

    CHAR scheme[INTERNET_MAX_SCHEME_LENGTH] = "";
    CHAR hostname[INTERNET_MAX_HOST_NAME_LENGTH] = "";
    CHAR username[INTERNET_MAX_USER_NAME_LENGTH] = "";
    CHAR password[INTERNET_MAX_PASSWORD_LENGTH] = "";
    CHAR urlpath[INTERNET_MAX_PATH_LENGTH] = "";

    URL_COMPONENTSA urlc = {
        sizeof(urlc),
        scheme, WW_COUNTOF(scheme),
        INTERNET_SCHEME_DEFAULT,
        hostname, WW_COUNTOF(hostname),
        0,
        username, WW_COUNTOF(username),
        password, WW_COUNTOF(password),
        urlpath, WW_COUNTOF(urlpath),
        NULL, 0
    };

    if (FALSE == InternetCrackUrlA(userParams->url, 0, 0, &urlc))
    {
        userParams->status = WW_ERR_URL_PARSE;
        WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
        return WW_FAILURE;
    }

    DWORD dwService = 0;
    DWORD dwFlags = 0;

    switch (urlc.nScheme)
    {
        case INTERNET_SCHEME_HTTP:
        case INTERNET_SCHEME_HTTPS:
            dwService = INTERNET_SERVICE_HTTP;
            break;
        default:
            userParams->errorcode = WW_ERR_UNKNOWN_SCHEME;
            WWLogA(userParams->logEnabled, WW_LOG_UNKNOWN_SCHEME,
                   urlc.lpszScheme);
            return WW_FAILURE;
            break;
    }

    HINTERNET hInet = InternetOpenA(userParams->userAgent,
                                     INTERNET_OPEN_TYPE_PRECONFIG,
                                     NULL, NULL, 0);
    if (NULL == hInet)
    {
        userParams->errorcode = WW_ERR_WININET_INIT;
        WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
        return WW_FAILURE;
    }

    HINTERNET hConn = InternetConnectA(hInet, urlc.lpszHostName, urlc.nPort,
                                        urlc.lpszUserName, urlc.lpszPassword,
                                        dwService, dwFlags, 0);
    if (NULL == hConn)
    {
        userParams->errorcode = WW_ERR_INTERNET_CONN;
        WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
        return WW_FAILURE;
    }

    INT iResult = WW_FAILURE;

    switch (urlc.nScheme)
    {
        case INTERNET_SCHEME_HTTP:
        case INTERNET_SCHEME_HTTPS:
            iResult = WWProcessHttpA(&urlc, hConn, userParams, privateParams);
            break;
        default:
            break;
    }

    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);
    return iResult;
}

WW_PRIVATE
INT
WWProcessHttpA(
    URL_COMPONENTSA* ptrUrlC,
    HINTERNET hConn,
    WW_PARAMSA* userParams,
    WW_PRIVATEPARAMSA* privateParams
)
{
    if (NULL == ptrUrlC)
    {
        userParams->errorcode = WW_ERR_EMPTY_URLC;
        return WW_FAILURE;
    }

    DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                    INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_NO_COOKIES |
                    INTERNET_FLAG_NO_UI;

    if (INTERNET_SCHEME_HTTPS == ptrUrlC->nScheme)
    {
        dwFlags |= INTERNET_FLAG_SECURE;
    }

    LPCSTR rgpszAcceptTypes[] = { "*/*", NULL };

    HINTERNET hReq = HttpOpenRequestA(hConn, NULL,
                                       ptrUrlC->lpszUrlPath, NULL, NULL,
                                       rgpszAcceptTypes, dwFlags, 0);

    if (NULL == hReq || FALSE == HttpSendRequestA(hReq, NULL, 0, NULL, 0))
    {
        userParams->errorcode = WW_ERR_HTTP_REQUEST;
        WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
        if (hReq != NULL)
        {
            InternetCloseHandle(hReq);
        }
        return WW_FAILURE;
    }

    DWORD dwQueryLength = 0;
    ZeroMemory(privateParams->szHeader, privateParams->headerSize);
    dwQueryLength = userParams->headerLength;
    if (HttpQueryInfoA(hReq,
        HTTP_QUERY_RAW_HEADERS_CRLF |
        HTTP_QUERY_FLAG_REQUEST_HEADERS,
        privateParams->szHeader, &dwQueryLength, 0) == FALSE) {
        WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
        InternetCloseHandle(hReq);
        return WW_FAILURE;
    }

    WWLogA(userParams->logEnabled, WW_LOG_HEADER, privateParams->szHeader);

    ZeroMemory(privateParams->szHeader, privateParams->headerSize);
    dwQueryLength = userParams->headerLength;
    if (HttpQueryInfoA(hReq, HTTP_QUERY_RAW_HEADERS_CRLF,
        privateParams->szHeader, &dwQueryLength, 0) == FALSE) {
        WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
        InternetCloseHandle(hReq);
        return WW_FAILURE;
    }

    WWLogA(userParams->logEnabled, WW_LOG_HEADER, privateParams->szHeader);

    DWORD dwStatusCode = 0;
    dwQueryLength = sizeof(dwStatusCode);

    if (HttpQueryInfoA(hReq, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
        &dwStatusCode, &dwQueryLength, 0) == FALSE) {
        WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
        InternetCloseHandle(hReq);
        return FALSE;
    }

    switch (dwStatusCode)
    {
        case HTTP_STATUS_OK:
            break;
        case HTTP_STATUS_MOVED:
        case HTTP_STATUS_REDIRECT:
        case HTTP_STATUS_REDIRECT_METHOD:
        case HTTP_STATUS_REDIRECT_KEEP_VERB:
            ZeroMemory(privateParams->szHeader, privateParams->headerSize);
            dwQueryLength = userParams->headerLength;
            if (FALSE == HttpQueryInfoA(hReq, HTTP_QUERY_LOCATION,
                                         privateParams->szHeader,
                                         &dwQueryLength, 0))
            {
                userParams->errorcode = WW_ERR_HTTP_QUERY_INFO;
                WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
                InternetCloseHandle(hReq);
                return WW_FAILURE;
            }
            InternetCloseHandle(hReq);
            InternetCloseHandle(hConn);
            userParams->url = privateParams->szHeader;
            return WWDownloadProcessA(userParams, privateParams);
            break;
        default:
            InternetCloseHandle(hReq);
            return WW_FAILURE;
            break;
    }

    if (WW_FAILURE ==
            WWMakeDownloadPathA(ptrUrlC->lpszUrlPath,
                                privateParams->capturedFileName,
                                WW_COUNTOF(privateParams->capturedFileName)))
    {
        return WW_FAILURE;
    }

    // Content disposition
    ZeroMemory(privateParams->szHeader, privateParams->headerSize);
    dwQueryLength = userParams->headerLength;

    if (TRUE == HttpQueryInfoA(hReq, HTTP_QUERY_CONTENT_DISPOSITION,
                                privateParams->szHeader, &dwQueryLength, 0))
    {
        LPSTR pattachment = strstr(privateParams->szHeader, "attachment;");
        if (NULL != pattachment)
        {
            LPSTR pfilename = strstr(pattachment, "filename");
            if (NULL != pfilename)
            {
                size_t ipath = strspn(pfilename + 8, " =\"");
                LPSTR pcontent = pfilename + 8 + ipath;
                LPSTR pch = pcontent;
                pch = strpbrk(pch, ";\"");
                if (NULL != pch)
                {
                    *pch = '\0';
                }
                pch = pcontent;
                while ((pch = strpbrk(pch, "\\/:*?\"<>|")) != NULL)
                {
                    *pch = '_';
                }
                if (strlen(pcontent) != 0)
                {
                    strncpy(privateParams->capturedFileName, pcontent,
                            WW_COUNTOF(privateParams->capturedFileName) - 1);
                    privateParams->capturedFileName[
                        WW_COUNTOF(privateParams->capturedFileName) - 1] = '\0';
                }
            }
        }
    }

    // Content length
    LONGLONG lDataLength = 0;
    DWORD bufferSize = sizeof(lDataLength);
    DWORD clFlags = HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER;
    if (FALSE == HttpQueryInfoA(hReq, clFlags, &lDataLength, &bufferSize, NULL))
    {
        InternetCloseHandle(hReq);
        return WW_FAILURE;
    }

    FILETIME ftLastModified = WW_STRUCT_NULL;
    SYSTEMTIME stLastModified = WW_STRUCT_NULL;
    dwQueryLength = sizeof(stLastModified);
    if (TRUE == HttpQueryInfoA(hReq,
                                HTTP_QUERY_LAST_MODIFIED |
                                HTTP_QUERY_FLAG_SYSTEMTIME,
                                &stLastModified, &dwQueryLength, 0))
    {
        SystemTimeToFileTime(&stLastModified, &ftLastModified);
    }

    INT iStatus = WWRetrieveDataA(hReq, lDataLength, &ftLastModified,
                                   userParams, privateParams);

    InternetCloseHandle(hReq);

    return iStatus;
}

WW_PRIVATE
INT
WWPrepareFilePathA(
    WW_PARAMSA* userParams,
    WW_PRIVATEPARAMSA* privateParams
)
{
    ZeroMemory(privateParams->fullFilePath, MAX_PATH);
    if (NULL != userParams->dstPath)
    {
        strncpy(privateParams->fullFilePath, userParams->dstPath,
            WW_STR_SYMSA_BUF(privateParams->fullFilePath));
    }
    if (NULL != userParams->outFileName)
    {
        if (0 == strlen(privateParams->fullFilePath))
        {
            strncpy(privateParams->fullFilePath, userParams->outFileName,
                WW_STR_SYMSA_BUF(privateParams->fullFilePath));
        }
        else
        {
            strncat(privateParams->fullFilePath, userParams->outFileName,
                WW_STR_SYMSA_BUF(privateParams->fullFilePath));
        }
    }
    else
    {
        if (0 == strlen(privateParams->capturedFileName))
        {
            return WW_FAILURE;
        }
        else
        {
            strncat(privateParams->fullFilePath,
                privateParams->capturedFileName,
                WW_STR_SYMSA_BUF(privateParams->fullFilePath));
        }
    }
    return WW_SUCCESS;
}

WW_PRIVATE
INT
WWRetrieveDataA(
    HINTERNET hFile,
    LONGLONG fileSize,
    CONST FILETIME* pftLastModified,
    WW_PARAMSA* userParams,
    WW_PRIVATEPARAMSA* privateParams
)
{
    static BYTE bufRead[0x10000];   //64KiB
    BOOL retRead;
    DWORD bytesRead, byteWrite;
    INT ratio = 0;
    WWPBARINFO* pbar = &userParams->progressBarData;
    pbar->szTotalInBytes = fileSize;
    pbar->szDownloadedInBytes = 0;
    LONGLONG szDownloadedPrev = 0;
    FILETIME st, st0, st1;
    CHAR filePathTemp[MAX_PATH] = "";

    if (WW_FAILURE == WWPrepareFilePathA(userParams, privateParams))
    {
        return WW_FAILURE;
    }

    HANDLE hFileN = CreateFileA(privateParams->fullFilePath, GENERIC_READ,
                                 0, NULL, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, 0);
    if (FALSE == WWIsFileModified(hFileN, fileSize, pftLastModified))
    {
        CloseHandle(hFileN);
        return WW_SUCCESS;
    }
    CloseHandle(hFileN);

    strncpy(filePathTemp, privateParams->fullFilePath,
            WW_STR_SYMSA_BUF(filePathTemp));
    strncat(filePathTemp, "~", WW_STR_SYMSA_BUF(filePathTemp));

    HANDLE hf = CreateFileA(privateParams->fullFilePath, GENERIC_WRITE, 0,
                             NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE == hf)
    {
        userParams->errorcode = WW_ERR_CREATE_FILE;
        WWLogA(userParams->logEnabled, WW_LOG_MODULE, NULL);
        return WW_FAILURE;
    }

    HANDLE hft = CreateFileA(filePathTemp, GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE == hft)
    {
        userParams->errorcode = WW_ERR_CREATE_FILE;
        WWLogA(userParams->logEnabled, WW_LOG_MODULE, NULL);
        CloseHandle(hf);
        return WW_FAILURE;
    }

    GetSystemTimeAsFileTime(&st);
    st0 = st;
    st1 = st;

    LPCSTR fileNamePtr = NULL;
    if (userParams->progressBarFlags & WW_PB_FILENAME)
    {
        fileNamePtr = userParams->outFileName;
        if (NULL == fileNamePtr)
        {
            fileNamePtr = privateParams->capturedFileName;
        }
    }
    while (TRUE)
    {
        ZeroMemory(bufRead, sizeof(bufRead));
        retRead = InternetReadFile(hFile, bufRead, sizeof(bufRead), &bytesRead);
        if (retRead)
        {
            if (bytesRead == 0)
            {
                break;
            }
        }
        else
        {
            WWLogA(userParams->logEnabled, WW_LOG_WININET, NULL);
            CloseHandle(hft);
            CloseHandle(hf);
            return WW_FAILURE;
        }

        if ((FALSE == WriteFile(hft, bufRead, bytesRead, &byteWrite, NULL)) ||
            bytesRead != byteWrite)
        {
            WWLogA(userParams->logEnabled, WW_LOG_MODULE, NULL);
            CloseHandle(hft);
            CloseHandle(hf);
            return WW_FAILURE;
        }

        pbar->szDownloadedInBytes += bytesRead;

        if (0 != pbar->szTotalInBytes)
        {
            ratio = (INT)((pbar->szDownloadedInBytes * 10000) / pbar->szTotalInBytes);
        }

        CHAR progressbar[64];
        ZeroMemory(progressbar, sizeof(progressbar));
        progressbar[0] = '[';
        for (INT i = 1; i < 24; i++)
        {
            if (ratio / 350 > i)
            {
                progressbar[i] = '#';
            }
            else
            {
                progressbar[i] = '-';
            }
        }
        progressbar[24] = ']';
        GetSystemTimeAsFileTime(&st1);
        ULONGLONG difftime = ((PULARGE_INTEGER)&st1)->QuadPart -
            ((PULARGE_INTEGER)&st0)->QuadPart;
        if ((difftime >= 10000000) || ((ratio != 0) && (ratio % 10000 == 0)))
        {
            DOUBLE diffsize =
                (DOUBLE)(pbar->szDownloadedInBytes - szDownloadedPrev) /
                ((DOUBLE)difftime / (DOUBLE)10000000);

            pbar->ulTimeElapsedInSecs = ((PULARGE_INTEGER)&st1)->QuadPart -
                ((PULARGE_INTEGER)&st)->QuadPart;
            ULONGLONG difftimeS = pbar->ulTimeElapsedInSecs;
            INT diffsec = (INT)((difftimeS - (difftimeS % 10000000)) / 10000000);
            INT speedUnitIndex = WWGetSizeUnitW(diffsize);

            st0 = st1;

            INT dwnSizeUnit = WWGetSizeUnitW((DOUBLE)pbar->szDownloadedInBytes);
            INT totlSizeUnit = WWGetSizeUnitW((DOUBLE)pbar->szTotalInBytes);

            DOUBLE downloadSpeed = diffsize / (diffsec != 0 ? diffsec : 1);
            ULONGLONG remainingSize = pbar->szTotalInBytes - pbar->szDownloadedInBytes;
            pbar->dETAInSecs = remainingSize / downloadSpeed;
            INT etaHours = (INT)(pbar->dETAInSecs / 3600);
            INT etaMinutes = (INT)((pbar->dETAInSecs - (etaHours * 3600)) / 60);
            INT etaSeconds = (INT)(pbar->dETAInSecs - (etaHours * 3600) - (etaMinutes * 60));

            if (userParams->progressCallback != NULL)
            {
                userParams->progressCallback(pbar,
                                             userParams->pCallbackData);
            }

            printf("\r");
            if (userParams->progressBarFlags & WW_PB_FILENAME)
            {
                printf("%s ", fileNamePtr);
            }
            if (userParams->progressBarFlags & WW_PB_PROGRESSBAR)
            {
                printf("%s ", progressbar);
            }
            if (userParams->progressBarFlags & WW_PB_PERCENTAGE)
            {
                printf("%3d.%02d%%; ",
                    (ratio - (ratio % 100)) / 100, (ratio % 100));
            }
            if (userParams->progressBarFlags & WW_PB_FILESIZE)
            {
                printf("%6.2f%ls /%6.2f%ls; ",
                    pbar->szDownloadedInBytes / sizeUnitW[dwnSizeUnit].size,
                    sizeUnitW[dwnSizeUnit].unit,
                    pbar->szTotalInBytes / sizeUnitW[totlSizeUnit].size,
                    sizeUnitW[totlSizeUnit].unit);
            }
            if (userParams->progressBarFlags & WW_PB_ELAPSEDTIME)
            {
                printf("%02d:%02d:%02d",
                    (diffsec - (diffsec % 3600)) / 3600,
                    ((diffsec % 3600) - (diffsec % 60)) / 60,
                    diffsec % 60);
            }
            if (userParams->progressBarFlags & WW_PB_SPEED)
            {
                printf("%6.2f%ls/s; ",
                    diffsize / sizeUnitW[speedUnitIndex].size,
                    sizeUnitW[speedUnitIndex].unit);
            }
            if (userParams->progressBarFlags & WW_PB_ETA)
            {
                printf("ETA: %02d:%02d:%02d ",
                    etaHours, etaMinutes, etaSeconds);
            }

            szDownloadedPrev = pbar->szDownloadedInBytes;
        }
    }

    printf("\n\n");

    if (pftLastModified != NULL)
    {
        if (SetFileTime(hft, NULL, NULL, pftLastModified) == FALSE)
        {
            WWLogA(userParams->logEnabled, WW_LOG_MODULE, NULL);
            CloseHandle(hft);
            CloseHandle(hf);
            return WW_FAILURE;
        }
    }

    CloseHandle(hft);
    CloseHandle(hf);

    if (MoveFileExA(filePathTemp, privateParams->fullFilePath,
        MOVEFILE_REPLACE_EXISTING) == FALSE)
    {
        WWLogA(userParams->logEnabled, WW_LOG_MODULE, NULL);
        return WW_FAILURE;
    }

    return WW_SUCCESS;
}

WW_PRIVATE
INT
WWMakeDownloadPathA(
    LPCSTR url,
    LPSTR path,
    DWORD len
)
{
    LPCSTR fnurl = strrchr(url, '/');
    if (fnurl == NULL || strlen(fnurl) == 1)
    {
        printf("ERROR : local path\n");
        return WW_FAILURE;
    }
    strncpy(path, fnurl + 1, len - 1);
    path[len - 1] = '\0';

    LPSTR ppath = strchr(path, '?');
    if (ppath != NULL)
    {
        *ppath = '\0';
    }

    LPSTR pfname = path;
    while ((pfname = strpbrk(pfname, "\\/:*?\"<>|")) != NULL)
    {
        *pfname = '_';
    }

    return WW_SUCCESS;
}