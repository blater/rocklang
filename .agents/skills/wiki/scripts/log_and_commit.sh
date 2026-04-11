#!/bin/sh

set -eu

usage() {
  cat >&2 <<'EOF'
Usage: log_and_commit.sh <type> <title> <path> [<path> ...]

Appends a wiki log entry using stdin as the body, then commits only the
specified paths plus wikiroot/log.md.
EOF
  exit 1
}

[ "$#" -ge 3 ] || usage

entry_type=$1
shift
title=$1
shift

repo_root=$(git rev-parse --show-toplevel 2>/dev/null) || {
  echo "Not inside a git repository." >&2
  exit 1
}

log_path="wikiroot/log.md"

cd "$repo_root"

[ -f "$log_path" ] || {
  echo "Missing $log_path" >&2
  exit 1
}

body_file=$(mktemp)
index_file=$(mktemp)

cleanup() {
  rm -f "$body_file" "$index_file"
}

trap cleanup EXIT HUP INT TERM

cat > "$body_file"

[ -s "$body_file" ] || {
  echo "Log body must be provided on stdin." >&2
  exit 1
}

GIT_INDEX_FILE="$index_file" git read-tree HEAD
GIT_INDEX_FILE="$index_file" git add --all -- "$@"

preexisting_changes=$(GIT_INDEX_FILE="$index_file" git diff --cached --name-only | wc -l | tr -d ' ')
[ "$preexisting_changes" -gt 0 ] || {
  echo "No changes found in the specified paths." >&2
  exit 1
}

date_stamp=$(date +%F)
count_placeholder="__WIKI_CHANGED_FILE_COUNT__"

{
  printf '\n## [%s] %s | %s\n\n' "$date_stamp" "$entry_type" "$title"
  printf 'Changed files: %s\n\n' "$count_placeholder"
  cat "$body_file"
  printf '\n'
} >> "$log_path"

GIT_INDEX_FILE="$index_file" git add --all -- "$@" "$log_path"

changed_files=$(GIT_INDEX_FILE="$index_file" git diff --cached --name-only | wc -l | tr -d ' ')

perl -0pi -e "s/$count_placeholder/$changed_files/" "$log_path"

GIT_INDEX_FILE="$index_file" git add --all -- "$@" "$log_path"
GIT_INDEX_FILE="$index_file" git commit -m "wiki $entry_type: $title"

git add --all -- "$@" "$log_path"

commit_id=$(git rev-parse HEAD)

printf 'Commit: %s\n' "$commit_id"
printf 'Changed files: %s\n' "$changed_files"
