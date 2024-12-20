#!/usr/bin/env perl
#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 2010 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution. The terms
# are also available at https://curl.se/docs/copyright.html.
#
# You may opt to use, copy, modify, merge, publish, distribute and/or sell
# copies of the Software, and permit persons to whom the Software is
# furnished to do so, under the terms of the COPYING file.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
# SPDX-License-Identifier: curl
#
###########################################################################
#
#

use strict;
use warnings;

# we may get the dir root pointed out
my $root=$ARGV[0] || ".";

my @incs = (
    "$root/include/curl/curl.h",
    "$root/include/curl/easy.h",
    "$root/include/curl/mprintf.h",
    "$root/include/curl/multi.h",
    "$root/include/curl/urlapi.h",
    "$root/include/curl/options.h",
    "$root/include/curl/header.h",
    );

my $verbose=0;
my $summary=0;
my $misses=0;

my @syms;
my %doc;
my %rem;

sub scanheader {
    my ($f)=@_;
    open H, "<$f" || die;
    my $first = "";
    while(<H>) {
        if (/^(^CURL_EXTERN .*)\(/) {
            my $decl = $1;
            $decl =~ s/\r$//;
            print "$decl\n";
        }
        elsif (/^(^CURL_EXTERN .*)/) {
            # handle two-line declarations
            my $decl = $1;
            $decl =~ s/\r$//;
            $first = $decl;
        }
        elsif($first) {
            if (/^(.*)\(/) {
                my $decl = $1;
                $decl =~ s/\r$//;
                $first .= $decl;
                print "$first\n";
            }
            $first = "";
        }
    }
    close H;
}

foreach my $i (@incs) {
    scanheader($i);
}
