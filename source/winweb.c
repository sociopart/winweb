/**
 * @file winweb.c
 * @authors SASAKI Nobuyuki, Ivan Korolev
 * @version 0.5b
 * @date 2022-2023
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