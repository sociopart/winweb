/**
 * @file winweb_private.h
 * @authors SASAKI Nobuyuki, Ivan Korolev
 * @version 0.5b
 * @date 2022-2023
 * @copyright MIT License
 * @brief Private header file for the WinWeb library.
 *
 *
 * 
 * 
 */

#ifndef WINWEB_PRIVATE_H
#define WINWEB_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif // WINWEB_PRIVATE_H