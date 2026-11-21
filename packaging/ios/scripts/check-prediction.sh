#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 [CoreDevice identifier]" >&2
    exit 2
}

if (( $# > 1 )); then
    usage
fi

scripts_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(git -C "$scripts_dir" rev-parse --show-toplevel)"
device_id="${1:-${KRITA_IOS_DEVICE:-}}"

if [[ -z "$device_id" ]]; then
    device_id="$(xcrun devicectl list devices | awk '
        /(connected|available)/ {
            for (field = 1; field <= NF; field++) {
                if (length($field) == 36 && $field ~ /^[[:xdigit:]-]+$/) {
                    print $field
                    exit
                }
            }
        }
    ')"
fi
if [[ -z "$device_id" ]]; then
    echo "error: no available CoreDevice found" >&2
    exit 1
fi

bundle_id="$(xcrun devicectl device info apps --device "$device_id" \
    | awk '$2 ~ /^local\.librepaint\.ipad/ { print $2; exit }')"
if [[ -z "$bundle_id" ]]; then
    echo "error: LibrePaint is not installed on the selected device" >&2
    echo "run build-incremental ios deploy first" >&2
    exit 2
fi

record_relative="Library/Application Support/latency.csv"
xcrun devicectl device process launch --device "$device_id" --terminate-existing \
    --environment-variables '{"LIBREPAINT_LATENCY_LOG":"Library/Application Support/latency.csv"}' \
    "$bundle_id"

seconds="${KRITA_IOS_PREDICTION_SECONDS:-120}"
if ! [[ "$seconds" =~ ^[0-9]+$ ]] || (( seconds < 1 )); then
    echo "error: KRITA_IOS_PREDICTION_SECONDS must be a positive integer" >&2
    exit 2
fi

prompt="Apple Pencilで新しい画像に線を20本以上描いてください。速く描いて止める線と、ゆっくり描く線を混ぜてください。"
remaining=$seconds
echo "${prompt}残り${remaining}秒"
while (( remaining > 30 )); do
    sleep 30
    remaining=$(( remaining - 30 ))
    echo "${prompt}残り${remaining}秒"
done
sleep "$remaining"

output_dir="$repo_root/build-ios/deploy"
mkdir -p "$output_dir"
stamp="$(date +%Y%m%d-%H%M%S)"
record_csv="$output_dir/prediction-${stamp}.csv"
record_log="$output_dir/prediction-${stamp}.log"

collect() {
    xcrun devicectl device copy from \
        --device "$device_id" \
        --domain-type appDataContainer \
        --domain-identifier "$bundle_id" \
        --source "$1" \
        --destination "$2"
}

if ! collect "$record_relative" "$record_csv"; then
    echo "error: could not collect the latency record" >&2
    exit 3
fi
if ! collect "Library/Application Support/librepaint.log" "$record_log"; then
    echo "error: could not collect the app log" >&2
    exit 3
fi
echo "record:  $record_csv"
echo "app log: $record_log"

exec python3 "$repo_root/scripts/perf/ios_prediction_check.py" "$record_csv"
