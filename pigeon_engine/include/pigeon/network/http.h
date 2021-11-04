#pragma once

// User-agent is IE11 on Windows 10
// change example.com is using a different site
#define HTTP_GET_HOMEPAGE \
"GET / HTTP/1.1\r\n" \
"Host: example.com\r\n" \
"Connection: close\r\n" \
"DNT: 1\r\n" \
"Upgrade-Insecure-Requests: 1\r\n" \
"User-Agent: Mozilla/5.0 (Windows NT 10.0; Trident/7.0; rv:11.0) like Gecko\r\n" \
"Accept: text/*, application/xhtml+xml\r\n" \
"Accept-Language: en-GB;q=0.9,en-US;q=0.8,en;q=0.7\r\n" \
"\r\n"
