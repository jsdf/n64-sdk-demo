#!/bin/bash
set -eu

WINE_PATH="/Applications/Wine Stable.app/Contents/Resources/wine/bin/wine"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
rm -f *.o
"$WINE_PATH" cmd /c $DIR/compile.bat
