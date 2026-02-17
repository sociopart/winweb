/**
 * @file winweb.hpp
 * @brief Header-only C++ wrapper for the WinWeb library.
 * @authors Ivan Korolev
 * @version 0.666
 * @note MIT License
 */

#pragma once
#include "winweb.h"
#include <string>
#include <vector>
#include <cstdint>

class WinWeb
{
public:
    WinWeb() = delete;

    /**
     * @brief HTTP response returned by query methods.
     *
     * Body data is owned by this struct; no manual free needed.
     */
    struct Response
    {
        DWORD statusCode = 0;
        std::vector<uint8_t> data;
        int errorcode = 0;

        /** Convenience accessor returning the body as a std::string. */
        std::string text() const
        {
            return std::string(reinterpret_cast<const char*>(data.data()), data.size());
        }
    };

    /**************************************************************************
     * Download
     *************************************************************************/

    /** Download a file to a full path (directory + filename combined). */
    static int Download(const std::string& url,
                        const std::string& fullFilePath,
                        DWORD flags = 0)
    {
        return WWDownloadW(s2w(url).c_str(), s2w(fullFilePath).c_str(), flags);
    }

    /** Download a file to a specific directory with a given filename. */
    static int DownloadAs(const std::string& url,
                          const std::string& dstPath,
                          const std::string& outFileName,
                          DWORD flags = 0)
    {
        return WWDownloadAsW(s2w(url).c_str(), s2w(dstPath).c_str(),
                             s2w(outFileName).c_str(), flags);
    }

    /** Download with full control via WW_PARAMSW. */
    static int DownloadEx(WW_PARAMSW& params)
    {
        return WWDownloadExW(&params);
    }

    /**************************************************************************
     * Query
     *************************************************************************/

    /**
     * Perform an HTTP request.
     *
     * @param url         Request URL (UTF-8).
     * @param verb        HTTP verb ("GET", "POST", "PUT", ...).
     * @param body        Raw request body, or nullptr for no body.
     * @param bodySize    Size of body in bytes.
     * @param contentType Content-Type header value, or empty to omit.
     * @return Response with status code, body data, and error code.
     */
    static Response Query(const std::string& url,
                          const std::string& verb,
                          const void* body = nullptr,
                          DWORD bodySize = 0,
                          const std::string& contentType = "")
    {
        std::wstring wurl  = s2w(url);
        std::wstring wverb = s2w(verb);
        std::wstring wct   = s2w(contentType);

        WW_RESPONSEW raw{};
        WWQueryW(wurl.c_str(), wverb.c_str(), body, bodySize,
                 wct.empty() ? nullptr : wct.c_str(), &raw);

        Response out;
        out.statusCode = raw.statusCode;
        out.errorcode  = raw.errorcode;
        if (raw.data != nullptr && raw.dataSize > 0)
            out.data.assign(raw.data, raw.data + raw.dataSize);

        WWFreeResponseW(&raw);
        return out;
    }

    /** Overload accepting body as a byte vector. */
    static Response Query(const std::string& url,
                          const std::string& verb,
                          const std::vector<uint8_t>& body,
                          const std::string& contentType = "")
    {
        return Query(url, verb, body.data(), static_cast<DWORD>(body.size()), contentType);
    }

    /** Query with full control via WW_REQUESTW. Response must be freed with WWFreeResponseW. */
    static int QueryEx(WW_REQUESTW& request, WW_RESPONSEW& response)
    {
        return WWQueryExW(&request, &response);
    }

private:
    /** Convert a UTF-8 std::string to std::wstring for WinAPI calls. */
    static std::wstring s2w(const std::string& s)
    {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring result(n - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, result.data(), n);
        return result;
    }
};
