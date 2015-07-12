#!/bin/sh

cd /pintos/src/threads/build
export PATH=/pintos/src/utils:$PATH
if [ "$1" == "gdb" ]; then
	pintos --qemu -v --gdb  -- -q run alarm-multiple
else 
    if [ "$1" != "" ]; then
        pgm="$1"
    else
        pgm="alarm-testsuit"
    fi
    echo "PGM: $pgm"
	
    case "$pgm" in 
       "alarm-testsuit")
	    pintos --qemu -v -- -q run alarm-single
	    pintos --qemu -v -- -q run alarm-multiple
	    pintos --qemu -v -- -q run alarm-simultaneous
	    pintos --qemu -v -- -q run alarm-zero
	    pintos --qemu -v -- -q run alarm-negative
            ;;
       "priority")
	    # pintos --qemu -v -- -q run alarm-priority
	    pintos --qemu -v -- -q run priority-change
            ;;
       *)   pintos --qemu -v -- -q run $pgm
            ;;		
    esac
fi

