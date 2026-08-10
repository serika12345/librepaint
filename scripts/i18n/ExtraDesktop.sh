#! /bin/sh

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
cd "$project_root" || exit 1

find . -name '*tag' -print | grep -v test.tag
