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

#pragma once

#include <IACore/JSON.hpp>

namespace IACore
{
    class HttpClient
    {
      public:
        enum class EHeaderType
        {
            ACCEPT,
            ACCEPT_CHARSET,
            ACCEPT_ENCODING,
            ACCEPT_LANGUAGE,
            AUTHORIZATION,
            CACHE_CONTROL,
            CONNECTION,
            CONTENT_LENGTH,
            CONTENT_TYPE,
            COOKIE,
            DATE,
            EXPECT,
            HOST,
            IF_MATCH,
            IF_MODIFIED_SINCE,
            IF_NONE_MATCH,
            ORIGIN,
            PRAGMA,
            PROXY_AUTHORIZATION,
            RANGE,
            REFERER,
            TE,
            UPGRADE,
            USER_AGENT,
            VIA,
            WARNING
        };

        enum class EResponseCode : INT32
        {
            // 1xx Informational
            CONTINUE = 100,
            SWITCHING_PROTOCOLS = 101,
            PROCESSING = 102,
            EARLY_HINTS = 103,

            // 2xx Success
            OK = 200,
            CREATED = 201,
            ACCEPTED = 202,
            NON_AUTHORITATIVE_INFORMATION = 203,
            NO_CONTENT = 204,
            RESET_CONTENT = 205,
            PARTIAL_CONTENT = 206,
            MULTI_STATUS = 207,
            ALREADY_REPORTED = 208,
            IM_USED = 226,

            // 3xx Redirection
            MULTIPLE_CHOICES = 300,
            MOVED_PERMANENTLY = 301,
            FOUND = 302,
            SEE_OTHER = 303,
            NOT_MODIFIED = 304,
            USE_PROXY = 305,
            TEMPORARY_REDIRECT = 307,
            PERMANENT_REDIRECT = 308,

            // 4xx Client Error
            BAD_REQUEST = 400,
            UNAUTHORIZED = 401,
            PAYMENT_REQUIRED = 402,
            FORBIDDEN = 403,
            NOT_FOUND = 404,
            METHOD_NOT_ALLOWED = 405,
            NOT_ACCEPTABLE = 406,
            PROXY_AUTHENTICATION_REQUIRED = 407,
            REQUEST_TIMEOUT = 408,
            CONFLICT = 409,
            GONE = 410,
            LENGTH_REQUIRED = 411,
            PRECONDITION_FAILED = 412,
            PAYLOAD_TOO_LARGE = 413,
            URI_TOO_LONG = 414,
            UNSUPPORTED_MEDIA_TYPE = 415,
            RANGE_NOT_SATISFIABLE = 416,
            EXPECTATION_FAILED = 417,
            IM_A_TEAPOT = 418,
            MISDIRECTED_REQUEST = 421,
            UNPROCESSABLE_ENTITY = 422,
            LOCKED = 423,
            FAILED_DEPENDENCY = 424,
            TOO_EARLY = 425,
            UPGRADE_REQUIRED = 426,
            PRECONDITION_REQUIRED = 428,
            TOO_MANY_REQUESTS = 429,
            REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
            UNAVAILABLE_FOR_LEGAL_REASONS = 451,

            // 5xx Server Error
            INTERNAL_SERVER_ERROR = 500,
            NOT_IMPLEMENTED = 501,
            BAD_GATEWAY = 502,
            SERVICE_UNAVAILABLE = 503,
            GATEWAY_TIMEOUT = 504,
            HTTP_VERSION_NOT_SUPPORTED = 505,
            VARIANT_ALSO_NEGOTIATES = 506,
            INSUFFICIENT_STORAGE = 507,
            LOOP_DETECTED = 508,
            NOT_EXTENDED = 510,
            NETWORK_AUTHENTICATION_REQUIRED = 511
        };

        using Header = KeyValuePair<EHeaderType, String>;

      public:
        HttpClient(IN CONST String &host);
        ~HttpClient();

      public:
        Expected<String, String> RawGet(IN CONST String &path, IN Span<CONST Header> headers,
                                        IN PCCHAR defaultContentType = "application/x-www-form-urlencoded");
        Expected<String, String> RawPost(IN CONST String &path, IN Span<CONST Header> headers, IN CONST String &body,
                                         IN PCCHAR defaultContentType = "application/x-www-form-urlencoded");

        template<typename _response_type>
        Expected<_response_type, String> JsonGet(IN CONST String &path, IN Span<CONST Header> headers);

        template<typename _payload_type, typename _response_type>
        Expected<_response_type, String> JsonPost(IN CONST String &path, IN Span<CONST Header> headers,
                                                  IN CONST _payload_type &body);

      public:
        STATIC String UrlEncode(IN CONST String &value);
        STATIC String UrlDecode(IN CONST String &value);

        STATIC String HeaderTypeToString(IN EHeaderType type);
        STATIC Header CreateHeader(IN EHeaderType key, IN CONST String &value);

        STATIC BOOL IsSuccessResponseCode(IN EResponseCode code);

      public:
        EResponseCode LastResponseCode()
        {
            return m_lastResponseCode;
        }

      private:
        PVOID m_client{};
        EResponseCode m_lastResponseCode;

      private:
        String PreprocessResponse(IN CONST String &response);
    };

    template<typename _response_type>
    Expected<_response_type, String> HttpClient::JsonGet(IN CONST String &path, IN Span<CONST Header> headers)
    {
        const auto rawResponse = RawGet(path, headers, "application/json");
        if (!rawResponse)
            return MakeUnexpected(rawResponse.error());
        if (LastResponseCode() != EResponseCode::OK)
            return MakeUnexpected(std::format("Server responded with code {}", (INT32) LastResponseCode()));
        return JSON::ParseToStruct<_response_type>(*rawResponse);
    }

    template<typename _payload_type, typename _response_type>
    Expected<_response_type, String> HttpClient::JsonPost(IN CONST String &path, IN Span<CONST Header> headers,
                                                          IN CONST _payload_type &body)
    {
        const auto encodedBody = IA_TRY(JSON::EncodeStruct(body));
        const auto rawResponse = RawPost(path, headers, encodedBody, "application/json");
        if (!rawResponse)
            return MakeUnexpected(rawResponse.error());
        if (LastResponseCode() != EResponseCode::OK)
            return MakeUnexpected(std::format("Server responded with code {}", (INT32) LastResponseCode()));
        return JSON::ParseToStruct<_response_type>(*rawResponse);
    }
} // namespace IACore