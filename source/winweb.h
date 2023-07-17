/**
 * @file winweb.h
 * @brief Header file for the WinWeb library.
 * @authors SASAKI Nobuyuki, Ivan Korolev
 * @version 0.5b
 * @date 2022-2023
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
#define WW_PB_PROGRESSBAR 0x00000001 // Show progress bar line
#define WW_PB_SPEED       0x00000002 // Show download speed
#define WW_PB_ETA         0x00000004 // Show ETA
#define WW_PB_FILESIZE    0x00000008 // Show file size
#define WW_PB_PERCENTAGE  0x00000010 // Show percentage
#define WW_PB_ELAPSEDTIME 0x00000020 // Show elapsed time
#define WW_PB_FILENAME    0x00000040 // Show filename
#define WW_DISPLAY_LOG    0x00000080 // Display logs
#define WW_FORCE_DOWNLOAD 0x00000100 // Re-download even when the file wasn't modified
#define WW_SHOW_PROGRESSBAR 0x00000110
#define WW_FLAGS_DEFAULT  NULL

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
} WW_PARAMSW;

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
INT WWDownloadA(LPCSTR url, LPCSTR path, LPCSTR out_filename, DWORD flags);

/**
 * @brief Function to download a file (Unicode version).
 *
 * This function downloads a file from the specified URL and saves it to the provided destination path.
 *
 * @param url The URL of the file to download.
 * @param dstPath The destination path for the downloaded file.
 * @param out_filename The output file name.
 * @param flags Additional flags.
 * @return 0 (WW_SUCCESS) on success, or 1 (WW_FAILURE) on failure.
 */
INT WWDownloadW(LPCWSTR url, LPCWSTR path, LPCWSTR out_filename, DWORD flags);

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

#ifdef __cplusplus
}
#endif

#endif // WINWEB_H
