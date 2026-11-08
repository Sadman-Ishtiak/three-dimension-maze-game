#!/usr/bin/env bash
# autopush.sh — auto-commit and push this repo to GitHub once every minute.
#
# Usage:
#   ./autopush.sh                          # every 60 seconds (default)
#   AUTOPUSH_INTERVAL=10 ./autopush.sh     # every 10 seconds
#
# Commit messages are generated from the actual staged diff, e.g.:
#   Update 3 files (+162 -3)
#     - main.py +12 -3
#     - README.md +147 (new)
# A single-file commit names the file directly:
#   Add parser.py +210
#
# Runs forever until you stop it with Ctrl+C.
# Commits respect .gitignore, so venv/ and caches are never pushed.

set -u

cd "$(dirname "$0")" || exit 1

INTERVAL="${AUTOPUSH_INTERVAL:-60}"
MAX_LIST=10   # files listed in full before the message truncates

stop() {
    echo
    echo "[$(date '+%H:%M:%S')] Stopped."
    exit 0
}
trap stop INT TERM

# Render one "path<TAB>status<TAB>added/deleted" entry as readable text.
render_entry() {
    local path st nums add del tag
    IFS=$'\t' read -r path st nums <<<"$1"
    tag=""
    case "$st" in
        A) tag=" (new)";;
        D) tag=" (deleted)";;
        B) tag=" (binary)";;
    esac
    if [ "$st" = "B" ]; then
        printf '%s%s' "$path" "$tag"
    else
        add="${nums%%/*}"; del="${nums##*/}"
        printf '%s +%d -%d%s' "$path" "$add" "$del" "$tag"
    fi
}

while true; do
    ts="$(date '+%Y-%m-%d %H:%M:%S')"

    # Commit any new or modified files (respects .gitignore).
    if [ -n "$(git status --porcelain)" ]; then
        git add -A
        if ! git diff --cached --quiet; then
            # Collect per-file stats: numstat gives +added/-deleted line
            # counts, name-status gives the change type (A/M/D/R).
            raw="$(paste <(git diff --cached --numstat -M) \
                          <(git diff --cached --name-status -M) \
                | awk -F'\t' '{
                    add=$1; del=$2; path=$3; st=substr($4,1,1)
                    if (add == "-") st = "B"
                    ta += add; td += del; n++
                    printf "%s\t%s\t", path, st
                    if (st != "B") printf "%d/%d", add, del
                    printf "\n"
                  }
                  END { printf "SUM\t%d\t%d\t%d\n", n, ta, td }')"

            n=0; tadd=0; tdel=0; entries=()
            while IFS=$'\t' read -r a b c d; do
                if [ "$a" = "SUM" ]; then
                    n="$b"; tadd="$c"; tdel="$d"
                else
                    entries+=("$a"$'\t'"$b"$'\t'"$c")
                fi
            done <<<"$raw"

            # Subject: name the file when there is one, summarise otherwise.
            if [ "$n" -eq 1 ]; then
                IFS=$'\t' read -r _ st _ <<<"${entries[0]}"
                case "$st" in
                    A) verb="Add";;
                    D) verb="Remove";;
                    R) verb="Rename";;
                    *) verb="Update";;
                esac
                subject="$verb $(render_entry "${entries[0]}")"
            else
                subject="Update $n files (+$tadd -$tdel)"
            fi

            # Body: one bullet per file, truncated past MAX_LIST.
            body=""; i=0
            for e in "${entries[@]}"; do
                i=$((i + 1))
                if [ "$i" -gt "$MAX_LIST" ]; then
                    body+="... and $((n - MAX_LIST)) more file(s)"$'\n'
                    break
                fi
                body+="- $(render_entry "$e")"$'\n'
            done
            body="${body%$'\n'}"

            if out="$(git commit -m "$subject" -m "$body" -m "Auto-commit at $ts" 2>&1)"; then
                echo "[$ts] Committed: $subject"
            else
                echo "[$ts] Commit failed: $out" >&2
            fi
        fi
    fi

    # Push every cycle so a previously failed push gets retried.
    if out="$(git push origin HEAD 2>&1)"; then
        case "$out" in
            *"Everything up-to-date"*) echo "[$ts] Nothing to push." ;;
            *) echo "[$ts] Pushed to GitHub." ;;
        esac
    else
        echo "[$ts] Push failed (retrying next cycle): $out" >&2
    fi

    sleep "$INTERVAL"
done
