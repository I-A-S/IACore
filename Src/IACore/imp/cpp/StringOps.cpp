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

#include <IACore/StringOps.hpp>

namespace IACore
{
    CONST String BASE64_CHAR_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    String StringOps::EncodeBase64(IN Span<CONST UINT8> data)
    {
        String result;
        result.reserve(((data.size() + 2) / 3) * 4);
        for (size_t i = 0; i < data.size(); i += 3)
        {
            uint32_t value = 0;
            INT32 num_bytes = 0;
            for (INT32 j = 0; j < 3 && (i + j) < data.size(); ++j)
            {
                value = (value << 8) | data[i + j];
                num_bytes++;
            }
            for (INT32 j = 0; j < num_bytes + 1; ++j)
            {
                if (j < 4)
                {
                    result += BASE64_CHAR_TABLE[(value >> (6 * (3 - j))) & 0x3F];
                }
            }
            if (num_bytes < 3)
            {
                for (INT32 j = 0; j < (3 - num_bytes); ++j)
                {
                    result += '=';
                }
            }
        }
        return result;
    }

    Vector<UINT8> StringOps::DecodeBase64(IN CONST String &data)
    {
        Vector<UINT8> result;

        CONST AUTO isBase64 = [](UINT8 c) { return (isalnum(c) || (c == '+') || (c == '/')); };

        INT32 in_len = data.size();
        INT32 i = 0, j = 0, in_ = 0;
        UINT8 tmpBuf0[4], tmpBuf1[3];

        while (in_len-- && (data[in_] != '=') && isBase64(data[in_]))
        {
            tmpBuf0[i++] = data[in_];
            in_++;
            if (i == 4)
            {
                for (i = 0; i < 4; i++)
                    tmpBuf0[i] = BASE64_CHAR_TABLE.find(tmpBuf0[i]);

                tmpBuf1[0] = (tmpBuf0[0] << 2) + ((tmpBuf0[1] & 0x30) >> 4);
                tmpBuf1[1] = ((tmpBuf0[1] & 0xf) << 4) + ((tmpBuf0[2] & 0x3c) >> 2);
                tmpBuf1[2] = ((tmpBuf0[2] & 0x3) << 6) + tmpBuf0[3];

                for (i = 0; (i < 3); i++)
                    result.push_back(tmpBuf1[i]);
                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j < 4; j++)
                tmpBuf0[j] = 0;

            for (j = 0; j < 4; j++)
                tmpBuf0[j] = BASE64_CHAR_TABLE.find(tmpBuf0[j]);

            tmpBuf1[0] = (tmpBuf0[0] << 2) + ((tmpBuf0[1] & 0x30) >> 4);
            tmpBuf1[1] = ((tmpBuf0[1] & 0xf) << 4) + ((tmpBuf0[2] & 0x3c) >> 2);
            tmpBuf1[2] = ((tmpBuf0[2] & 0x3) << 6) + tmpBuf0[3];

            for (j = 0; (j < i - 1); j++)
                result.push_back(tmpBuf1[j]);
        }

        return result;
    }
} // namespace IACore