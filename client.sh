#!/usr/bin/bash

iperf -c 224.0.0.200 -p 8000 -B 127.0.0.1 -u -T 32 -t 4096 -i 1
