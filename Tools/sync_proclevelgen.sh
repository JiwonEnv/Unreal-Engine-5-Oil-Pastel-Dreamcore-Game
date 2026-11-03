#!/usr/bin/env bash
# Mirrors Plugins/ProcLevelGen into the UE5-Procedural-Level-Generation repo and commits it there with the same message.
# Usage: Tools/sync_proclevelgen.sh "feat: ..."   (run after committing the plugin change here)
set -euo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)/Plugins/ProcLevelGen"
DEST_REPO="${PROCLEVELGEN_REPO:-/e/Project/UE5-Procedural-Level-Generation}"
DEST="$DEST_REPO/Plugins/ProcLevelGen"
MESSAGE="${1:?commit message required}"

[ -d "$SRC" ] || { echo "missing $SRC" >&2; exit 1; }
[ -d "$DEST_REPO/.git" ] || { echo "missing repo $DEST_REPO" >&2; exit 1; }

# Fresh copy without build output, so deleted files disappear there too.
rm -rf "$DEST"
mkdir -p "$DEST"
(cd "$SRC" && tar --exclude=Binaries --exclude=Intermediate -cf - .) | (cd "$DEST" && tar -xf -)

cd "$DEST_REPO"
git add -A Plugins/ProcLevelGen
if git diff --cached --quiet; then
	echo "nothing to sync"
	exit 0
fi
git commit -q -m "$MESSAGE"
git log --oneline -1
