#ifndef HEADER_CURL_TOOL_LIBINFO_H
#define HEADER_CURL_TOOL_LIBINFO_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/
#include "tool_setup.h"

/* global variable declarations, for libcurl run-time info */


extern curl_version_info_data *curlinfo;
extern const char * const *built_in_protos;
extern size_t proto_count;

extern const char *proto_file;
extern const char *proto_ftp;
extern const char *proto_ftps;
extern const char *proto_http;
extern const char *proto_https;
extern const char *proto_rtsp;
extern const char *proto_scp;
extern const char *proto_sftp;
extern const char *proto_tftp;

CURLcode get_libcurl_info(void);
const char *proto_token(const char *proto);

#endif /* HEADER_CURL_TOOL_LIBINFO_H */
