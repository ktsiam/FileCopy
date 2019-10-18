#!/bin/bash
for ((i=0;i<100;i++)) do
/usr/bin/time ./client-e2e-check comp117-01 4 5 SRC
done >client.txt 2>&1
