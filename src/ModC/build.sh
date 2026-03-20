#! /bin/sh
set -e

ModCScriptDir="$(dirname "$0")"
ModCRepoRoot="${ModCScriptDir}/../.."

ModCFlags="-std=c99 -Wall -Wextra -Wpedantic -Werror -Wno-sign-compare -fsanitize=undefined -g3"

ModCIncludes="-I${ModCRepoRoot}/External -I${ModCRepoRoot}/External/uthash/src -I${ModCRepoRoot}/src"

# Preprocessor output
# gcc ${ModCFlags} -E -P ${ModCIncludes} "${ModCScriptDir}/main.c" -o "${ModCScriptDir}/main.i"

# Normal output
gcc ${ModCFlags} ${ModCIncludes} "${ModCScriptDir}/main.c" -o "${ModCScriptDir}/Build/ModC"

# Use preprocessor output as input
# gcc ${ModCFlags} ${ModCIncludes} "${ModCScriptDir}/main.i" -o "${ModCScriptDir}/Build/ModC"
