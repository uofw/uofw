#include <common_imp.h>
#include <libparse_http.h>
#include <threadman_user.h>

SCE_MODULE_INFO("SceParseHTTPheader_Library", SCE_MODULE_ATTR_EXCLUSIVE_LOAD | SCE_MODULE_ATTR_EXCLUSIVE_START, 1, 1);
SCE_SDK_VERSION(SDK_VERSION);

// Address 0x00000840
static const u8 ascii_map[128] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x8, 0x8, 0x8, 0x8, 0x8, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x18, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x10, 0x10, 0x10, 0x10, 0x20
};

#define IS_LOWERCASE_ASCII(c)           (ascii_map[c] & 0x2)
#define IS_DIGIT_ASCII(c)               (ascii_map[c] & 0x4)
#define IS_WHITESPACE_ASCII(c)          (ascii_map[c] & 0x8)
#define UPPERCASE(c)                    (c - 0x20)
#define PARSE_ASCII_DIGIT(c)            (c - 0x30)

static s32 parseHttpVersion(u8* str, u32 length, s32 *major, s32 *minor);
static s32 parseHttpResponseStatusCode(u8* str, s32 length, s32* response_code);
static s32 stringCompareIgnoreCase(u8* str0, u8* str1, s32 length);
static u32 getLineRemainder(u8* str, u32 len, u8** remainder, u32* remainder_length);
static s32 stringLength(u8* str);
static s32 stringCompare(u8* str0, u8* str1, u32 length);

// Subroutine sceParseHttp_8077A433 - Address 0x00000000
s32 sceParseHttpStatusLine(
    u8* str, s32 length, s32* httpVersionMajor, s32* httpVersionMinor,
    s32* httpStatusCode, u8** httpStatus, u32* httpStatusLength)
{
    if(sceKernelCheckThreadStack() < 992)
    {
        return SCE_PARSE_HTTP_THREAD_ERROR;
    }
    if(str == 0)
    {
        return SCE_PARSE_HTTP_INVALID_RESPONSE;
    }
    if(httpVersionMajor == 0 || httpVersionMinor == 0 || httpStatusCode == 0 || httpStatus == 0 || httpStatusLength == 0)
    {
        return SCE_PARSE_HTTP_NULL_ARG;
    }
    s32 parsedCount0 = parseHttpVersion(str, length, httpVersionMajor, httpVersionMinor);
    if(parsedCount0 < 0)
    {
        return parsedCount0;
    }
    s32 parsedCount1 = parseHttpResponseStatusCode(str + parsedCount0, length - parsedCount0, httpStatusCode);
    if(parsedCount1 < 0)
    {
        return parsedCount1;
    }
    s32 parsedCount2 = getLineRemainder(str + parsedCount0 + parsedCount1, length - parsedCount0 - parsedCount1, httpStatus, httpStatusLength);
    if(parsedCount2 < 0)
    {
        return parsedCount2;
    }
    return parsedCount0 + parsedCount1 + parsedCount2;
}

// Subroutine sceParseHttp_AD7BFDEF - Address 0x00000118
s32 sceParseHttpResponseHeader(u8* str, s32 length, u8* header_name, u8** header_value, u32* value_length)
{
    if(sceKernelCheckThreadStack() < 960)
    {
        return SCE_PARSE_HTTP_THREAD_ERROR;
    }
    if(str == 0)
    {
        return SCE_PARSE_HTTP_INVALID_RESPONSE;
    }
    if(header_name == 0 || header_value == 0 || value_length == 0)
    {
        return SCE_PARSE_HTTP_NULL_ARG;
    }
    if(length == 0)
    {
        return SCE_PARSE_HTTP_HEADER_NOT_FOUND;
    }

    s32 count = 0;
    s32 field_name_length = stringLength(header_name);
    while(count < length)
    {
        while(IS_WHITESPACE_ASCII(str[count]))
        {
            count++;
        }
        if(count < length && stringCompareIgnoreCase(str + count, header_name, field_name_length) == 0
            && str[count + field_name_length] == ':')
        {
            count += field_name_length + 1;
            while(IS_WHITESPACE_ASCII(str[count]))
            {
                count++;
            }
            s32 value_count = 0;
            while(str[count + value_count] != '\n')
            {
                value_count++;
            }
            *header_value = str + count;
            *value_length = value_count;
            return count + value_count;
        }
        else
        {
            while(count < length && str[count] != '\n')
            {
                count++;
            }
        }
    }

    return SCE_PARSE_HTTP_HEADER_NOT_FOUND;
}

// Subroutine sub_000003A0 - Address 0x000003A0
static s32 parseHttpVersion(u8* str, u32 length, s32 *major, s32 *minor)
{
    u8 http[] = "HTTP/";
    if(length < 9)
    {
        return SCE_PARSE_HTTP_INVALID_RESPONSE;
    }
    if(stringCompare(str, http, 5) != 0)
    {
        return SCE_PARSE_HTTP_INVALID_RESPONSE;
    }
    if(!IS_DIGIT_ASCII(str[5]))
    {
        return SCE_PARSE_HTTP_INVALID_RESPONSE;
    }
    *major = PARSE_ASCII_DIGIT(str[5]);
    if(str[6] != '.')
    {
        return SCE_PARSE_HTTP_INVALID_RESPONSE;
    }
    *minor = PARSE_ASCII_DIGIT(str[7]);
    if(str[8] != ' ')
    {
        return SCE_PARSE_HTTP_INVALID_RESPONSE;
    }
    return 9;
}

// Subroutine sub_00000510 - Address 0x00000510
static s32 parseHttpResponseStatusCode(u8* str, s32 length, s32* response_code)
{
    if(length < 4 || !IS_DIGIT_ASCII(str[0]) || !IS_DIGIT_ASCII(str[1]) || !IS_DIGIT_ASCII(str[2]))
    {
        return SCE_PARSE_HTTP_INVALID_RESPONSE;
    }
    *response_code = str[0] * 100 + str[1] * 10 + str[2] - 5328;
    return 3;
}

// Subroutine sub_000005A0 - Address 0x000005A0
static s32 stringCompareIgnoreCase(u8* str0, u8* str1, s32 length)
{
    for(s32 i = 0; i < length; i++)
    {
        u8 char0 = str0[i];
        u8 char1 = str1[i];
        if(IS_LOWERCASE_ASCII(char0))
        {
            char0 = UPPERCASE(char0);
        }
        if(IS_LOWERCASE_ASCII(char1))
        {
            char1 = UPPERCASE(char1);
        }
        if(char1 - char0 != 0)
        {
            return char1 - char0;
        }
        if(str0[i] == 0 && str1[i] == 0)
        {
            return 0;
        }
    }
    return 0;
}

// Subroutine sub_0000061C - Address 0x0000061C
static u32 getLineRemainder(u8* str, u32 len, u8** remainder, u32* remainder_length)
{
    u32 count = 0;
    while(str[count] != 10) // \n
    {
        count++;
        if(count > len)
        {
            return SCE_PARSE_HTTP_INVALID_RESPONSE;
        }
    }

    if(count != 0)
    {
        if(str[count - 1] == 13) // \r
        {
            *remainder_length = count - 1;
        }
        else
        {
            *remainder_length = count;
        }
    }
    else {
        *remainder_length = 0;
    }
    *remainder = str;
    return count + 1;
}

// Subroutine sub_00000698 - Address 0x00000698
static s32 stringLength(u8* str)
{
    if(str == 0)
    {
        return 0;
    }
    s32 count = 0;
    while (str[count] != 0)
    {
        count++;
    }
    return count;
}

// Subroutine sub_000006CC - Address 0x000006CC
static s32 stringCompare(u8* str0, u8* str1, u32 length) {
    if(str0 == NULL || str1 == NULL)
    {
        if(str0 == str1)
        {
            return 0;
        }
        if(str0 == NULL)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    for(u32 i = 0; i < length; i++)
    {
        s8 char0 = str0[i];
        s8 char1 = str1[i];
        if(char0 != char1)
        {
            return char0 - char1;
        }
    }
    return 0;
}
