// IACore-OSS; The Core Library for All IA Open Source Projects
// Copyright (C) 2025 IAS (ias@iasoft.dev)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <IACore/HttpClient.hpp>
#include <IACore/DataOps.hpp>

#include <httplib.h>

namespace IACore
{
    httplib::Headers BuildHeaders(IN Span<CONST HttpClient::Header> headers, IN PCCHAR defaultContentType)
    {
        httplib::Headers out;
        bool hasContentType = false;

        for (const auto &h : headers)
        {
            std::string key = HttpClient::HeaderTypeToString(h.first);
            out.emplace(key, h.second);

            if (h.first == HttpClient::EHeaderType::CONTENT_TYPE)
                hasContentType = true;
        }

        if (!hasContentType && defaultContentType)
            out.emplace("Content-Type", defaultContentType);
        return out;
    }

    HttpClient::HttpClient(IN CONST String &host)
        : m_client(new httplib::Client(host)), m_lastResponseCode(EResponseCode::INTERNAL_SERVER_ERROR)
    {
    }

    HttpClient::~HttpClient()
    {
        if (m_client)
            delete static_cast<httplib::Client *>(m_client);
    }

    String HttpClient::PreprocessResponse(IN CONST String &response)
    {
        const auto responseBytes = Span<CONST UINT8>{(PCUINT8) response.data(), response.size()};
        const auto compression = DataOps::DetectCompression(responseBytes);
        switch (compression)
        {
        case DataOps::CompressionType::Gzip: {
            const auto data = DataOps::GZipInflate(responseBytes);
            if (!data)
                return response;
            return String((PCCHAR) data->data(), data->size());
        }

        case DataOps::CompressionType::Zlib: {
            const auto data = DataOps::ZlibInflate(responseBytes);
            if (!data)
                return response;
            return String((PCCHAR) data->data(), data->size());
        }

        case DataOps::CompressionType::None:
        default:
            break;
        }
        return response;
    }

    Expected<String, String> HttpClient::RawGet(IN CONST String &path, IN Span<CONST Header> headers,
                                                IN PCCHAR defaultContentType)
    {
        auto httpHeaders = BuildHeaders(headers, defaultContentType);

        static_cast<httplib::Client *>(m_client)->enable_server_certificate_verification(false);
        auto res = static_cast<httplib::Client *>(m_client)->Get(
            (!path.empty() && path[0] != '/') ? ('/' + path).c_str() : path.c_str(), httpHeaders);

        if (res)
        {
            m_lastResponseCode = static_cast<EResponseCode>(res->status);
            if (res->status >= 200 && res->status < 300)
                return PreprocessResponse(res->body);
            else
                return MakeUnexpected(std::format("HTTP Error {} : {}", res->status, res->body));
        }

        return MakeUnexpected(std::format("Network Error: {}", httplib::to_string(res.error())));
    }

    Expected<String, String> HttpClient::RawPost(IN CONST String &path, IN Span<CONST Header> headers,
                                                 IN CONST String &body, IN PCCHAR defaultContentType)
    {
        auto httpHeaders = BuildHeaders(headers, defaultContentType);

        String contentType = defaultContentType;
        if (httpHeaders.count("Content-Type"))
        {
            const auto t = httpHeaders.find("Content-Type");
            contentType = t->second;
            httpHeaders.erase(t);
        }

        static_cast<httplib::Client *>(m_client)->set_keep_alive(true);
        static_cast<httplib::Client *>(m_client)->enable_server_certificate_verification(false);
        auto res = static_cast<httplib::Client *>(m_client)->Post(
            (!path.empty() && path[0] != '/') ? ('/' + path).c_str() : path.c_str(), httpHeaders, body,
            contentType.c_str());

        if (res)
        {
            m_lastResponseCode = static_cast<EResponseCode>(res->status);
            if (res->status >= 200 && res->status < 300)
                return PreprocessResponse(res->body);
            else
                return MakeUnexpected(std::format("HTTP Error {} : {}", res->status, res->body));
        }

        return MakeUnexpected(std::format("Network Error: {}", httplib::to_string(res.error())));
    }
} // namespace IACore

namespace IACore
{
    HttpClient::Header HttpClient::CreateHeader(IN EHeaderType key, IN CONST String &value)
    {
        return std::make_pair(key, value);
    }

    String HttpClient::UrlEncode(IN CONST String &value)
    {
        std::stringstream escaped;
        escaped.fill('0');
        escaped << std::hex << std::uppercase;

        for (char c : value)
        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~')
                escaped << c;
            else
                escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }

        return escaped.str();
    }

    String HttpClient::UrlDecode(IN CONST String &value)
    {
        String result;
        result.reserve(value.length());

        for (size_t i = 0; i < value.length(); ++i)
        {
            if (value[i] == '%' && i + 2 < value.length())
            {
                std::string hexStr = value.substr(i + 1, 2);
                char decodedChar = static_cast<char>(std::strtol(hexStr.c_str(), nullptr, 16));
                result += decodedChar;
                i += 2;
            }
            else if (value[i] == '+')
                result += ' ';
            else
                result += value[i];
        }

        return result;
    }

    String HttpClient::HeaderTypeToString(IN EHeaderType type)
    {
        switch (type)
        {
        case EHeaderType::ACCEPT:
            return "Accept";
        case EHeaderType::ACCEPT_CHARSET:
            return "Accept-Charset";
        case EHeaderType::ACCEPT_ENCODING:
            return "Accept-Encoding";
        case EHeaderType::ACCEPT_LANGUAGE:
            return "Accept-Language";
        case EHeaderType::AUTHORIZATION:
            return "Authorization";
        case EHeaderType::CACHE_CONTROL:
            return "Cache-Control";
        case EHeaderType::CONNECTION:
            return "Connection";
        case EHeaderType::CONTENT_LENGTH:
            return "Content-Length";
        case EHeaderType::CONTENT_TYPE:
            return "Content-Type";
        case EHeaderType::COOKIE:
            return "Cookie";
        case EHeaderType::DATE:
            return "Date";
        case EHeaderType::EXPECT:
            return "Expect";
        case EHeaderType::HOST:
            return "Host";
        case EHeaderType::IF_MATCH:
            return "If-Match";
        case EHeaderType::IF_MODIFIED_SINCE:
            return "If-Modified-Since";
        case EHeaderType::IF_NONE_MATCH:
            return "If-None-Match";
        case EHeaderType::ORIGIN:
            return "Origin";
        case EHeaderType::PRAGMA:
            return "Pragma";
        case EHeaderType::PROXY_AUTHORIZATION:
            return "Proxy-Authorization";
        case EHeaderType::RANGE:
            return "Range";
        case EHeaderType::REFERER:
            return "Referer";
        case EHeaderType::TE:
            return "TE";
        case EHeaderType::UPGRADE:
            return "Upgrade";
        case EHeaderType::USER_AGENT:
            return "User-Agent";
        case EHeaderType::VIA:
            return "Via";
        case EHeaderType::WARNING:
            return "Warning";
        }
    }

    BOOL HttpClient::IsSuccessResponseCode(IN EResponseCode code)
    {
        return (INT32) code >= 200 && (INT32) code < 300;
    }
} // namespace IACore