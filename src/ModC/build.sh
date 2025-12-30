#! /bin/sh
set -e

ModCScriptDir="$(dirname "$0")"
ModCRepoRoot="${ModCScriptDir}/../.."

ModCFlags="-std=c99 -Wall -Wextra -Wpedantic -Werror -Wno-sign-compare -fsanitize=undefined -g3"

# Preprocessor output
# gcc ${ModCFlags} "-E -CC -P" -I${ModCRepoRoot}/External -I${ModCRepoRoot}/src "${ModCScriptDir}/main.c"


# Normal output
gcc ${ModCFlags} -I${ModCRepoRoot}/External -I${ModCRepoRoot}/src "${ModCScriptDir}/main.c" -o "${ModCScriptDir}/Build/ModC"

# Use preprocessor output as input
# gcc ${ModCFlags} -I${ModCRepoRoot}/External -I${ModCRepoRoot}/src "${ModCScriptDir}/main.i" -o "${ModCScriptDir}/Build/ModC"
