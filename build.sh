#!/bin/bash
set -eu
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
rm -f *.o
/Applications/Wine\ Stable.app/Contents/Resources/wine/bin/wine cmd /c $DIR/compile.bat
