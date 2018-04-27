#!/bin/bash

VIEWER=build/vs12/Debug/visusviewer.exe

# slaves 
$VIEWER --network-rcv 10001 &
$VIEWER --network-rcv 10002 &
$VIEWER --network-rcv 10003 &
$VIEWER --network-rcv 10004 &

# master (--network-snd url "ortho_bounds(x y w h)" "screen_bounds(x y w h)" aspect_ratio
$VIEWER \
  --network-snd http://localhost:10001 "0.0 0.0 0.5 0.5" "  0   500 500 500" 1.0 \
  --network-snd http://localhost:10002 "0.5 0.0 0.5 0.5" "500   500 500 500" 1.0 \
  --network-snd http://localhost:10003 "0.5 0.5 0.5 0.5" "500     0 500 500" 1.0 \
  --network-snd http://localhost:10004 "0.0 0.5 0.5 0.5" "  0     0 500 500" 1.0 \
  # 
  