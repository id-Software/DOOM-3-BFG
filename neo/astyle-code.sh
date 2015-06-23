#!/bin/sh

# keep the following in sync with our supplied astyle binaries!
OUR_ASTYLE_VERSION="2.05.1"

print_usage () {
	echo "By default, this script only works on Linux i686 or amd64 (with our supplied astyle binaries)"
	echo "You can use your own astyle binary by setting the ASTYLE_BIN environment variable before calling this script"
	echo "  e.g.: ASTYLE_BIN=/usr/bin/astyle $0"
	echo "But please make sure it's version $OUR_ASTYLE_VERSION because other versions may format differently!"
}

if [ -z "$ASTYLE_BIN" ]; then

	if [ `uname -s` != "Linux" ]; then
		print_usage
		exit 1
	fi

	case "`uname -m`" in
		i?86 | x86 ) ASTYLE_SUFFIX="x86" ;;
		amd64 | x86_64 ) ASTYLE_SUFFIX="x86_64" ;;
		* ) print_usage ; exit 1  ;;
	esac

	ASTYLE_BIN="./astyle.$ASTYLE_SUFFIX"
fi

ASTYLE_VERSION=$($ASTYLE_BIN --version | grep -o -e "[[:digit:]\.]*")

if [ "$ASTYLE_VERSION" != "$OUR_ASTYLE_VERSION" ]; then
	echo "ERROR: $ASTYLE_BIN has version $ASTYLE_VERSION, but we want $OUR_ASTYLE_VERSION"
	echo "       (Unfortunately, different versions of astyle produce slightly different formatting.)"
	exit 1
fi

$ASTYLE_BIN -v --formatted --options=astyle-options.ini --exclude="libs" --recursive "*.h"
$ASTYLE_BIN -v --formatted --options=astyle-options.ini --exclude="libs" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" \
		--exclude="sys/win32/win_cpu.cpp" --exclude="sys/win32/win_main.cpp" --recursive "*.cpp"
