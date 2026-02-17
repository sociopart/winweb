/**
 * @file winweb.h
 * @brief Header file for the WinWeb library.
 * @authors SASAKI Nobuyuki, Ivan Korolev
 * @version 0.666
 * @date 2022-2026
 * @note MIT License
 *
 * This header file provides functions and definitions to interact with the 
 * WinAPI and facilitate file downloads.
 * It includes declarations for public functions, constants, and data 
 * structures used in the library.
 */

#ifndef WINWEB_H
#define WINWEB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>
#include <WinInet.h>
#ifdef _MSC_VER
#pragma comment(lib, "wininet.lib")
#endif

/**
 * @brief Constant definitions.
 */
#define WW_DEFAULT_USER_AGENTA "Winweb/0.5b"
#define WW_DEFAULT_USER_AGENTW L"Winweb/0.5b"
#define WW_DEFAULT_REDIRECT_LIMIT 4
#define WW_DEFAULT_HEADER_LENGTH 16384
#define WW_SUCCESS 0
#define WW_FAILURE 1

/**
 * @brief Constants for different flags.
 */
#define WW_PB_PROGRESSBAR   0x00000001 // Show progress bar line
#define WW_PB_SPEED         0x00000002 // Show download speed
#define WW_PB_ETA           0x00000004 // Show ETA
#define WW_PB_FILESIZE      0x00000008 // Show file size
#define WW_PB_PERCENTAGE    0x00000010 // Show percentage
#define WW_PB_ELAPSEDTIME   0x00000020 // Show elapsed time
#define WW_PB_FILENAME      0x00000040 // Show filename
#define WW_SHOW_LOG         0x00000080 // Display logs
#define WW_FORCE_DOWNLOAD   0x00000100 // Ignore if file wasn't modified
#define WW_SHOW_PROGRESSBAR 0x00000110 // Show progress bar information
#define WW_FLAGS_DEFAULT    NULL       // No extra flags

/**
* @brief Enumerations for different error codes.
*/

enum E_WW_ERRORCODES {
    WW_ERR_UNKNOWN = -1,
    WW_ERR_NOERROR,
    WW_ERR_NO_URL,
    WW_ERR_REDIRS_EXCEEDED,
    WW_ERR_URL_PARSE,
    WW_ERR_UNKNOWN_SCHEME,
    WW_ERR_WININET_INIT,
    WW_ERR_INTERNET_CONN,
    WW_ERR_MALLOC,
    WW_ERR_EMPTY_URLC,
    WW_ERR_HTTP_REQUEST,
    WW_ERR_HTTP_QUERY_INFO,
    WW_ERR_NO_DOWNLOAD_PATH,
    WW_ERR_CREATE_FILE,
};

/**
 * @brief Enumerations for different work statuses in the WinWeb library.
 */
enum E_WW_WORKSTATUS {
    WW_STATUS_INIT,        /**< Work status: Initialized */
    WW_STATUS_ERROR,       /**< Work status: Error */
    WW_STATUS_DOWNLOAD,    /**< Work status: Downloading */
    WW_STATUS_SUCCESS      /**< Work status: Successful */
};

/**
 * @brief Structure representing information for the progress bar.
 */
typedef struct {
    ULONGLONG ulTimeElapsedInSecs;
    ULONGLONG szDownloadedInBytes;  /**< Size of downloaded data in bytes */
    ULONGLONG szTotalInBytes;       /**< Total size of the data in bytes */
    DOUBLE dETAInSecs;
} WWPBARINFO;

/**
 * @brief Callback function type for download progress reporting.
 *
 * @param pProgressData Pointer to progress bar information.
 * @param pUserData     User-provided context pointer.
 */
typedef VOID (*WW_PROGRESS_CALLBACK)(CONST WWPBARINFO* pProgressData,
                                     LPVOID pUserData);

/**
 * @brief Structure representing parameters for the WinWeb library functions (ANSI version).
 */
typedef struct {
    INT status;                       /**< Work status */
    INT errorcode;                    /**< Stores error code */
    LPCSTR url;                       /**< URL of the file to download */
    LPCSTR dstPath;                   /**< Destination path for the downloaded file */
    LPCSTR outFileName;               /**< Output file name */
    LPCSTR userAgent;                 /**< User agent string */
    UINT maxRedirectLimit;            /**< Maximum redirect limit */
    UINT headerLength;                /**< Length of additional header */
    INT logEnabled;                   /**< Flag to enable/disable logging */
    INT progressBarEnabled;           /**< Flag to enable/disable progress bar */
    INT forceDownload;                /**< Flag to enable/disable force downloading */
    DWORD progressBarFlags;           /**< Progress bar flags */
    WWPBARINFO progressBarData;       /**< Structure for progress bar data */
    WW_PROGRESS_CALLBACK progressCallback; /**< Optional progress callback */
    LPVOID pCallbackData;             /**< User context for progress callback */
} WW_PARAMSA;

/**
 * @brief Structure representing parameters for the WinWeb library functions (Unicode version).
 */
typedef struct {
    INT status;                       /**< Work status */
    INT errorcode;                    /**< Stores error code */
    LPCWSTR url;                      /**< URL of the file to download */
    LPCWSTR dstPath;                  /**< Destination path for the downloaded file */
    LPCWSTR outFileName;              /**< Output file name */
    LPCWSTR userAgent;                /**< User agent string */
    UINT maxRedirectLimit;            /**< Maximum redirect limit */
    UINT headerLength;                /**< Length of additional header */
    INT logEnabled;                   /**< Flag to enable/disable logging */
    INT progressBarEnabled;           /**< Flag to enable/disable progress bar */
    INT forceDownload;                /**< Flag to enable/disable force downloading */
    DWORD progressBarFlags;           /**< Progress bar flags */
    WWPBARINFO progressBarData;       /**< Progress bar data */
    WW_PROGRESS_CALLBACK progressCallback; /**< Optional progress callback */
    LPVOID pCallbackData;             /**< User context for progress callback */
} WW_PARAMSW;

/**
 * @brief Structure representing an HTTP response (ANSI version).
 */
typedef struct {
    DWORD statusCode;                 /**< HTTP status code */
    LPBYTE data;                      /**< Response body (library-allocated) */
    SIZE_T dataSize;                  /**< Size of response body in bytes */
    INT errorcode;                    /**< Error code on failure */
} WW_RESPONSEA;

/**
 * @brief Structure representing an HTTP response (Unicode version).
 */
typedef struct {
    DWORD statusCode;                 /**< HTTP status code */
    LPBYTE data;                      /**< Response body (library-allocated) */
    SIZE_T dataSize;                  /**< Size of response body in bytes */
    INT errorcode;                    /**< Error code on failure */
} WW_RESPONSEW;

/**
 * @brief Structure representing an HTTP request (ANSI version).
 */
typedef struct {
    LPCSTR url;                       /**< Request URL */
    LPCSTR verb;                      /**< HTTP method (GET, POST, PUT, DELETE, etc.) */
    LPCSTR userAgent;                 /**< User agent string */
    LPCSTR contentType;               /**< Content-Type header for POST/PUT */
    LPCVOID body;                     /**< Request body data */
    DWORD bodySize;                   /**< Size of request body in bytes */
    LPCSTR headers;                   /**< Additional custom headers */
    UINT maxRedirectLimit;            /**< Maximum redirect limit */
    BOOL logEnabled;                  /**< Flag to enable/disable logging */
} WW_REQUESTA;

/**
 * @brief Structure representing an HTTP request (Unicode version).
 */
typedef struct {
    LPCWSTR url;                      /**< Request URL */
    LPCWSTR verb;                     /**< HTTP method (GET, POST, PUT, DELETE, etc.) */
    LPCWSTR userAgent;                /**< User agent string */
    LPCWSTR contentType;              /**< Content-Type header for POST/PUT */
    LPCVOID body;                     /**< Request body data */
    DWORD bodySize;                   /**< Size of request body in bytes */
    LPCWSTR headers;                  /**< Additional custom headers */
    UINT maxRedirectLimit;            /**< Maximum redirect limit */
    BOOL logEnabled;                  /**< Flag to enable/disable logging */
} WW_REQUESTW;

/**
 * @brief Function to download a file (ANSI version).
 *
 * This function downloads a file from the specified URL and saves it to the provided destination path.
 *
 * @param url The URL of the file to download.
 * @param dstPath The destination path for the downloaded file.
 * @param out_filename The output file name.
 * @param flags Additional flags.
 * @return 0 (WW_SUCCESS) on success, or 1 (WW_FAILURE) on failure.
 */
INT WWDownloadA(LPCSTR url, LPCSTR path, LPCSTR outFileName, DWORD flags);

/**
 * @brief Function to download a file (Unicode version).
 *
 * This function downloads a file from the specified URL and saves it to the provided destination path.
 *
 * @param url The URL of the file to download.
 * @param fullFilePath The destination path for the downloaded file (including extension).
 * @param flags Additional flags.
 * @return 0 (WW_SUCCESS) on success, or 1 (WW_FAILURE) on failure.
 */

INT WWDownloadW(LPCWSTR url, LPCWSTR fullFilePath, DWORD flags);
/**
 * @brief Function to download a file as specific filename (Unicode version).
 *
 * This function downloads a file from the specified URL and saves it to the provided destination path.
 *
 * @param url The URL of the file to download.
 * @param dstPath The destination path for the downloaded file.
 * @param outFileName The output file name.
 * @param flags Additional flags.
 * @return 0 (WW_SUCCESS) on success, or 1 (WW_FAILURE) on failure.
 */
INT WWDownloadAsW(LPCWSTR url, LPCWSTR path, LPCWSTR outFileName, DWORD flags);

/**
 * @brief Function to download a file with extended parameters (ANSI version).
 *
 * This function downloads a file with extended parameters provided in the WW_PARAMSA structure.
 *
 * @param params The WW_PARAMSA structure containing the download parameters.
 * @return 0 (WW_SUCCESS) on success, or 1 (WW_FAILURE) on failure.
 */
INT WWDownloadExA(WW_PARAMSA* params);

/**
 * @brief Function to download a file with extended parameters (Unicode version).
 *
 * This function downloads a file with extended parameters provided in the WW_PARAMSW structure.
 *
 * @param params The WW_PARAMSW structure containing the download parameters.
 * @return 0 (WW_SUCCESS) on success, or 1 (WW_FAILURE) on failure.
 */
INT WWDownloadExW(WW_PARAMSW* params);

/**
 * @brief Perform an HTTP request (ANSI version).
 *
 * Pass NULL for body to send a GET request.
 */
INT WWQueryA(LPCSTR url, LPCSTR verb, LPCVOID body, DWORD bodySize,
             LPCSTR contentType, WW_RESPONSEA* response);

/**
 * @brief Perform an HTTP request (Unicode version).
 *
 * Pass NULL for body to send a GET request.
 */
INT WWQueryW(LPCWSTR url, LPCWSTR verb, LPCVOID body, DWORD bodySize,
             LPCWSTR contentType, WW_RESPONSEW* response);

/**
 * @brief Perform an HTTP request with extended parameters (ANSI version).
 */
INT WWQueryExA(WW_REQUESTA* request, WW_RESPONSEA* response);

/**
 * @brief Perform an HTTP request with extended parameters (Unicode version).
 */
INT WWQueryExW(WW_REQUESTW* request, WW_RESPONSEW* response);

/**
 * @brief Free response data allocated by query functions (ANSI version).
 */
VOID WWFreeResponseA(WW_RESPONSEA* response);

/**
 * @brief Free response data allocated by query functions (Unicode version).
 */
VOID WWFreeResponseW(WW_RESPONSEW* response);

#ifdef __cplusplus
}
#endif

#endif // WINWEB_H
