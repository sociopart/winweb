/**
 * @file winweb.hpp
 * @brief Header-only C++ wrapper for the WinWeb library.
 * @authors Ivan Korolev
 * @version 0.666
 * @note MIT License
 */

#pragma once
#include "../../source/winweb.h"
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
                          const std::string& contentType = "",
                          const std::string& headers = "",
                          DWORD timeoutMs = 0)
    {
        std::wstring wurl  = s2w(url);
        std::wstring wverb = s2w(verb);
        std::wstring wct   = s2w(contentType);
        std::wstring whdr  = s2w(headers);

        WW_REQUESTW request{};
        request.url              = wurl.c_str();
        request.verb             = wverb.c_str();
        request.userAgent        = WW_DEFAULT_USER_AGENTW;
        request.contentType      = wct.empty() ? nullptr : wct.c_str();
        request.body             = body;
        request.bodySize         = bodySize;
        request.headers          = whdr.empty() ? nullptr : whdr.c_str();
        request.maxRedirectLimit = WW_DEFAULT_REDIRECT_LIMIT;
        request.logEnabled       = FALSE;
        request.connectTimeoutMs = timeoutMs;
        request.sendTimeoutMs    = timeoutMs;
        request.receiveTimeoutMs = timeoutMs;

        WW_RESPONSEW raw{};
        WWQueryExW(&request, &raw);

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
                          const std::string& contentType = "",
                          const std::string& headers = "",
                          DWORD timeoutMs = 0)
    {
        return Query(url, verb, body.data(), static_cast<DWORD>(body.size()),
                     contentType, headers, timeoutMs);
    }

    /** Query with full control via WW_REQUESTW. Response must be freed with WWFreeResponseW. */
    static int QueryEx(WW_REQUESTW& request, WW_RESPONSEW& response)
    {
        return WWQueryExW(&request, &response);
    }

    /** Return remote Content-Length via HEAD, following redirects. */
    static bool GetRemoteFileSize(const std::string& url,
                                  size_t& outSize,
                                  DWORD timeoutMs = 5000)
    {
        ULONGLONG remoteSize = 0;
        std::wstring wurl = s2w(url);
        if (WWGetRemoteFileSizeW(wurl.c_str(), &remoteSize, timeoutMs) != WW_SUCCESS)
            return false;

        outSize = static_cast<size_t>(remoteSize);
        return true;
    }

    /** Ping host via ICMP. Returns round-trip time in ms, or 0 on failure. */
    static int Ping(const std::string& host, DWORD timeoutMs = 1000)
    {
        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            return 0;

        addrinfo hints{};
        hints.ai_family = AF_INET;
        addrinfo* res = nullptr;
        if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || !res)
        {
            WSACleanup();
            return 0;
        }

        DWORD ip = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr.s_addr;
        freeaddrinfo(res);
        WSACleanup();

        HANDLE hIcmp = IcmpCreateFile();
        if (hIcmp == INVALID_HANDLE_VALUE)
            return 0;

        char sendData[32] = {};
        DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
        std::vector<BYTE> replyBuf(replySize);

        DWORD sent = IcmpSendEcho(hIcmp, ip, sendData, sizeof(sendData),
                                  nullptr, replyBuf.data(), replySize, timeoutMs);
        IcmpCloseHandle(hIcmp);

        if (sent == 0)
            return 0;
        return static_cast<int>(reinterpret_cast<ICMP_ECHO_REPLY*>(replyBuf.data())->RoundTripTime);
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
