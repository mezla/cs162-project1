#!/bin/sh

cd /pintos/src/threads/build
export PATH=/pintos/src/utils:$PATH
if [ $1 == "gdb" ]; then
	pintos --qemu -v --gdb  -- -q run alarm-multiple
else 
	pintos --qemu -v -- -q run alarm-multiple
fi
