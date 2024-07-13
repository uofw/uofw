#ifndef LIBPARSE_HTTP_H
#define LIBPARSE_HTTP_H

#include <common_header.h>

#define SCE_PARSE_HTTP_THREAD_ERROR         0x80410005
#define SCE_PARSE_HTTP_HEADER_NOT_FOUND     0x80432025
#define SCE_PARSE_HTTP_INVALID_RESPONSE     0x80432060
#define SCE_PARSE_HTTP_NULL_ARG             0x804321FE

/**
 * Parse an HTTP status line and write results to appropriate supplied parameters.
 * A typical status line looks like: "HTTP/1.1 404 Not Found".
 * 
 * @param str Pointer to HTTP status string.
 * @param length Length of HTTP status string.
 * @param httpVersionMajor Pointer where HTTP version major will be written.
 * @param httpVersionMinor Pointer where HTTP version minor will be written.
 * @param httpStatusCode Pointer where HTTP status code will be written.
 * @param httpStatus Pointer where reference to HTTP status text will be stored.
 * @param httpStatusLength Pointer where HTTP status text length will be written.
 * 
 * @return Number of parsed chars on success. SCE_PARSE_HTTP_INVALID_RESPONSE
 * on invalid HTTP status format. SCE_PARSE_HTTP_NULL_ARG on NULL pointers
 * passed in parameters. SCE_PARSE_HTTP_THREAD_ERROR on thread error(?)
 */
s32 sceParseHttpStatusLine(
    u8* str, s32 length, s32* httpVersionMajor, s32* httpVersionMinor,
    s32* httpStatusCode, u8** httpStatus, u32* httpStatusLength);


/**
 * Extract HTTP header value from HTTP response.
 * 
 * @param str Pointer to HTTP headers string.
 * @param length Length of HTTP headers string.
 * @param header_name Requested header name.
 * @param header_value Pointer where reference to requested header value will be stored.
 * @param value_length Pointer where header value length will be written.
 * 
 * @return Number of parsed chars on success. SCE_PARSE_HTTP_HEADER_NOT_FOUND
 * when requested header is not found. SCE_PARSE_HTTP_INVALID_RESPONSE on invalid
 * HTTP response format. SCE_PARSE_HTTP_NULL_ARG on NULL pointers
 * passed in parameters. SCE_PARSE_HTTP_THREAD_ERROR on thread error(?)
 */
s32 sceParseHttpResponseHeader(
    u8* str, s32 length, u8* header_name, u8** header_value, u32* value_length);

#endif /* LIBPARSE_HTTP_H */
