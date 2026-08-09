#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat >&2 <<'EOF'
usage: build-librepaint-incremental.sh <command> [arguments]

commands:
  configure                  Configure the persistent Ninja build tree.
  plan                       Show the work Ninja would perform.
  build [--allow-large]      Incrementally build LibrePaint; refuse a large rebuild by default.
  bootstrap                  Build a new baseline, allowing the initial full build.
  deploy [device-id]         Incrementally build, then package and install through AltStore.
  path                       Print the build tree selected by the pinned Nix configuration.
EOF
    exit 2
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(git -C "$script_dir" rev-parse --show-toplevel)"
script_path="$script_dir/build-librepaint-incremental.sh"

# Record a source-independent LibrePaint environment once per Nix/configuration revision.
# Reusing the profile avoids copying the roughly 575 MiB source tree into the
# Nix store after every ordinary source edit. The environment still exposes
# the exact compiler, target closure, host tools, and CMake flags of the app.
if [[ "${KRITA_IOS_INCREMENTAL_ENV:-0}" != "1" ]]; then
    cd "$repo_root"

    environment_fingerprint="$({
        printf 'schema=2\n'
        {
            printf '%s\n' \
                "$repo_root/flake.nix" \
                "$repo_root/flake.lock" \
                "$repo_root/packaging/ios/versions.env"
            find \
                "$repo_root/nix/ios" \
                "$repo_root/packaging/ios/deps" \
                "$repo_root/packaging/ios/frameworks" \
                "$repo_root/packaging/ios/manifests" \
                "$repo_root/packaging/ios/qt" \
                -type f -print
        } | LC_ALL=C sort | while IFS= read -r input_path; do
            printf 'path=%s\n' "${input_path#"$repo_root/"}"
            shasum -a 256 "$input_path" | awk '{ print "sha256=" $1 }'
        done
    } | shasum -a 256 | awk '{ print $1 }')"

    environment_profile_root="$repo_root/build-ios/nix-profiles/krita-ios-incremental"
    environment_profile="$environment_profile_root/profile"
    environment_fingerprint_path="$environment_profile_root/fingerprint"
    environment_profile_refreshed=0

    validate_incremental_environment_profile() {
        local reference references
        if ! references="$(nix-store --query --requisites "$environment_profile")"; then
            echo "error: failed to inspect incremental environment closure" >&2
            return 1
        fi
        while IFS= read -r reference; do
            case "${reference##*/}" in
                *-krita-ios-source)
                    echo "error: incremental environment retains repository source: $reference" >&2
                    return 1
                    ;;
            esac
        done <<<"$references"
    }

    recorded_environment_fingerprint=""
    if [[ -f "$environment_fingerprint_path" ]]; then
        recorded_environment_fingerprint="$(<"$environment_fingerprint_path")"
    fi
    if [[ ! -e "$environment_profile" ]] \
        || [[ "$recorded_environment_fingerprint" != "$environment_fingerprint" ]]; then
        mkdir -p "$environment_profile_root"
        echo "creating pinned incremental environment: $environment_profile" >&2
        nix develop .#librepaint-ios-incremental \
            --profile "$environment_profile" \
            --command true
        nix profile wipe-history --profile "$environment_profile" >/dev/null
        environment_profile_refreshed=1
    fi

    validate_incremental_environment_profile
    if [[ "$environment_profile_refreshed" == "1" ]]; then
        printf '%s\n' "$environment_fingerprint" \
            >"$environment_fingerprint_path.tmp"
        mv "$environment_fingerprint_path.tmp" "$environment_fingerprint_path"
    fi

    exec nix develop "$environment_profile" --command env \
        KRITA_IOS_INCREMENTAL_ENV=1 \
        "$script_path" "$@"
fi

command_name="${1:-}"
[[ -n "$command_name" ]] || usage
shift

if [[ -z "${cmakeFlags:-}" ]]; then
    echo "error: the librepaint-ios-app development environment did not provide cmakeFlags" >&2
    exit 1
fi
if [[ -z "${KRITA_IOS_TOOLCHAIN_IDENTITY:-}" ]]; then
    echo "error: the pinned iOS toolchain identity is unavailable" >&2
    exit 1
fi

read -r -a cmake_args <<<"$cmakeFlags"

cmake_path="$(command -v cmake)"
ninja_path="$(command -v ninja)"
config_schema=1
config_fingerprint="$({
    printf 'schema=%s\n' "$config_schema"
    printf 'toolchain=%s\n' "$KRITA_IOS_TOOLCHAIN_IDENTITY"
    printf 'cmake=%s\n' "$cmake_path"
    printf 'ninja=%s\n' "$ninja_path"
    printf 'flags=%s\n' "$cmakeFlags"
} | sha256sum | awk '{print $1}')"

build_root="${KRITA_IOS_INCREMENTAL_ROOT:-$repo_root/build-ios/krita/device-incremental}"
build_dir="${KRITA_IOS_BUILD_DIR:-$build_root/${config_fingerprint:0:16}}"
if [[ "$build_dir" != /* ]]; then
    build_dir="$repo_root/$build_dir"
fi
build_dir="$(realpath -m -- "$build_dir")"
if [[ "$build_dir" == "/" || "$build_dir" == "$repo_root" ]]; then
    echo "error: unsafe incremental build tree: $build_dir" >&2
    exit 1
fi
marker_path="$build_dir/.krita-ios-incremental-config"
lock_dir="${build_dir}.lock"
target=krita
configure_header_backup=""

prepare_environment() {
    # Match the target-pure Nix builder while keeping debug paths stable across
    # separate `nix develop` invocations.
    unset \
        AR AS CC CPP CXX LD LDFLAGS NM OBJC OBJCXX \
        CMAKE_FRAMEWORK_PATH CMAKE_INCLUDE_PATH CMAKE_LIBRARY_PATH \
        CMAKE_PREFIX_PATH CMAKE_PREFIX_PATH_FOR_BUILD NIXPKGS_CMAKE_PREFIX_PATH \
        CPATH IPHONEOS_DEPLOYMENT_TARGET LIBRARY_PATH \
        MACOSX_DEPLOYMENT_TARGET NIX_CFLAGS_COMPILE NIX_LDFLAGS \
        QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH \
        QT_ADDITIONAL_PACKAGES_PREFIX_PATH QT_OPTIONAL_TOOLS_PATH

    export SOURCE_DATE_EPOCH=1
    export ZERO_AR_DATE=1
    export CFLAGS="-ffile-prefix-map=$repo_root=/build/source -fdebug-prefix-map=$repo_root=/build/source"
    export CXXFLAGS="$CFLAGS"
    export ASMFLAGS="$CFLAGS"
    export OBJCFLAGS="$CFLAGS"
    export OBJCXXFLAGS="$CXXFLAGS"
    export NINJA_STATUS='[%f/%t] '

    target_roots=""
    for arg in "${cmake_args[@]}"; do
        case "$arg" in
            -DCMAKE_PREFIX_PATH=*) target_roots="${arg#-DCMAKE_PREFIX_PATH=}" ;;
        esac
    done
    if [[ -z "$target_roots" ]]; then
        echo "error: the Nix app recipe did not provide a target CMAKE_PREFIX_PATH" >&2
        exit 1
    fi

    pkg_config_libdir=""
    old_ifs="$IFS"
    IFS=';'
    read -r -a target_root_list <<<"$target_roots"
    IFS="$old_ifs"
    for root in "${target_root_list[@]}"; do
        [[ -n "$pkg_config_libdir" ]] && pkg_config_libdir+=:
        pkg_config_libdir+="$root/lib/pkgconfig:$root/share/pkgconfig"
    done
    export PKG_CONFIG_PATH=""
    export PKG_CONFIG_DIR=""
    export PKG_CONFIG_LIBDIR="$pkg_config_libdir"
    export PKG_CONFIG_SYSROOT_DIR=""
}

cmake_flag_value() {
    local wanted_name="$1"
    local arg assignment flag_name
    for arg in "${cmake_args[@]}"; do
        [[ "$arg" == -D*=* ]] || continue
        assignment="${arg#-D}"
        flag_name="${assignment%%=*}"
        flag_name="${flag_name%%:*}"
        if [[ "$flag_name" == "$wanted_name" ]]; then
            printf '%s\n' "${assignment#*=}"
            return 0
        fi
    done
    return 1
}

check_cache_value() {
    local name="$1"
    local expected="$2"
    local count actual
    count="$(grep -Ec "^$name:[^=]*=" "$build_dir/CMakeCache.txt" || true)"
    if [[ "$count" != "1" ]]; then
        echo "error: expected one Krita cache entry for $name; found $count" >&2
        exit 1
    fi
    actual="$(sed -n "s/^$name:[^=]*=//p" "$build_dir/CMakeCache.txt")"
    if [[ "$actual" != "$expected" ]]; then
        echo "error: Krita cache $name is '$actual'; expected '$expected'" >&2
        exit 1
    fi
}

normalize_krita_build_dir() {
    local config_header="$build_dir/KoConfig.h"
    local configured_line="#define KRITA_BUILD_DIR \"$build_dir\""
    local normalized_line='#define KRITA_BUILD_DIR "/build"'
    local configured_count normalized_count line

    if [[ ! -f "$config_header" ]]; then
        echo "error: configured Krita tree omitted KoConfig.h" >&2
        exit 1
    fi
    configured_count="$(grep -Fxc "$configured_line" "$config_header" || true)"
    normalized_count="$(grep -Fxc "$normalized_line" "$config_header" || true)"
    if [[ "$configured_count" == "0" && "$normalized_count" == "1" ]]; then
        return 0
    fi
    if [[ "$configured_count" != "1" || "$normalized_count" != "0" ]]; then
        echo "error: KoConfig.h contains an unexpected KRITA_BUILD_DIR definition" >&2
        exit 1
    fi

    : >"$config_header.tmp"
    while IFS= read -r line || [[ -n "$line" ]]; do
        if [[ "$line" == "$configured_line" ]]; then
            printf '%s\n' "$normalized_line" >>"$config_header.tmp"
        else
            printf '%s\n' "$line" >>"$config_header.tmp"
        fi
    done <"$config_header"
    mv "$config_header.tmp" "$config_header"
}

validate_configure_contract() {
    local name expected
    local -a cache_names=(
        CMAKE_SYSTEM_NAME
        CMAKE_OSX_ARCHITECTURES
        CMAKE_OSX_DEPLOYMENT_TARGET
        CMAKE_OSX_SYSROOT
        BUILD_TESTING
        BUILD_WITH_QT6
        KRITA_IOS_BUILD_PLUGINS
        KRITA_IOS_BUILD_QML_MODULES
        KF6_HOST_TOOLING
        Qt6LinguistTools_DIR
    )
    for name in "${cache_names[@]}"; do
        if ! expected="$(cmake_flag_value "$name")"; then
            echo "error: the Nix app recipe omitted the expected CMake flag: $name" >&2
            exit 1
        fi
        check_cache_value "$name" "$expected"
    done
    normalize_krita_build_dir
}

write_marker() {
    {
        printf 'schema=%s\n' "$config_schema"
        printf 'fingerprint=%s\n' "$config_fingerprint"
        printf 'toolchain=%s\n' "$KRITA_IOS_TOOLCHAIN_IDENTITY"
        printf 'cmake=%s\n' "$cmake_path"
        printf 'ninja=%s\n' "$ninja_path"
    } >"$marker_path.tmp"
    mv "$marker_path.tmp" "$marker_path"
}

verify_marker() {
    if [[ ! -f "$marker_path" ]]; then
        echo "error: incremental build tree is not configured: $build_dir" >&2
        echo "run: $script_path configure" >&2
        exit 1
    fi
    if ! grep -Fxq "fingerprint=$config_fingerprint" "$marker_path"; then
        echo "error: incremental build tree has a different configuration fingerprint" >&2
        echo "expected a separate baseline at: $build_dir" >&2
        exit 1
    fi
}

validate_configure_destination() {
    if [[ -e "$build_dir" && ! -d "$build_dir" ]]; then
        echo "error: incremental build tree is not a directory: $build_dir" >&2
        exit 1
    fi
    if [[ -f "$marker_path" ]]; then
        verify_marker
    elif [[ -d "$build_dir" ]] \
        && [[ -n "$(find "$build_dir" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
        echo "error: refusing to configure a non-empty unowned build tree: $build_dir" >&2
        echo "choose an empty KRITA_IOS_BUILD_DIR or use the fingerprint-selected tree" >&2
        exit 1
    fi
}

validate_host() {
    local host_check_output
    if ! host_check_output="$(
        PATH="/usr/bin:/bin:/usr/sbin:/sbin:$PATH" \
            "$script_dir/check-host.sh" --toolchain-only-strict 2>&1
    )"; then
        printf '%s\n' "$host_check_output" >&2
        exit 1
    fi
}

release_lock() {
    if [[ -n "$configure_header_backup" ]]; then
        rm -f "$configure_header_backup"
    fi
    rmdir "$lock_dir" 2>/dev/null || true
}

acquire_lock() {
    mkdir -p "$(dirname "$lock_dir")"
    if ! mkdir "$lock_dir" 2>/dev/null; then
        echo "error: another incremental operation owns: $lock_dir" >&2
        echo "if no operation is running, remove this empty stale lock directory" >&2
        exit 1
    fi
    trap release_lock EXIT
    trap 'exit 129' HUP
    trap 'exit 130' INT
    trap 'exit 143' TERM
    validate_host
    python3 "$repo_root/packaging/ios/scripts/replace-brand-art-with-white.py" \
        --audit-ios-classification
    python3 "$repo_root/packaging/ios/scripts/audit-default-resource-bundle.py"
    python3 "$repo_root/packaging/ios/scripts/audit-static-dependency-resources.py"
    python3 "$repo_root/packaging/ios/scripts/audit-user-visible-branding.py" \
        --source-root "$repo_root"
    python3 "$repo_root/packaging/ios/scripts/audit-ios-compatibility-identifiers.py" \
        --source-root "$repo_root"
}

configure_tree() {
    validate_configure_destination
    prepare_environment
    mkdir -p "$build_dir"
    configure_header_backup=""
    if [[ -f "$build_dir/KoConfig.h" ]] \
        && grep -Fxq '#define KRITA_BUILD_DIR "/build"' "$build_dir/KoConfig.h"; then
        configure_header_backup="$(mktemp "${TMPDIR:-/tmp}/krita-ios-KoConfig.XXXXXX")"
        cp -p "$build_dir/KoConfig.h" "$configure_header_backup"
    fi
    if ! cmake -S "$repo_root" -B "$build_dir" \
        "${cmake_args[@]}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; then
        return 1
    fi
    validate_configure_contract
    if [[ -n "$configure_header_backup" ]]; then
        if cmp -s "$configure_header_backup" "$build_dir/KoConfig.h"; then
            touch -r "$configure_header_backup" "$build_dir/KoConfig.h"
        fi
        rm -f "$configure_header_backup"
        configure_header_backup=""
    fi
    write_marker
}

refresh_tree() {
    verify_marker
    prepare_environment

    # Avoid a 20–30 second CMake configure on ordinary source-only changes.
    # If a CMake input is newer, regenerate explicitly before calculating the
    # build plan so the large-rebuild guard sees the post-regeneration graph.
    if ! regeneration_plan="$(ninja -C "$build_dir" -n build.ninja 2>&1)"; then
        printf '%s\n' "$regeneration_plan" >&2
        exit 1
    fi
    if grep -Eq '^\[[0-9]+/[0-9]+\]' <<<"$regeneration_plan"; then
        echo "CMake inputs changed; regenerating the Ninja graph..."
        configure_tree
    fi
    validate_configure_contract
}

plan_output=""
planned_steps=0
create_plan() {
    verify_marker
    if ! plan_output="$(ninja -C "$build_dir" -n "$target" 2>&1)"; then
        printf '%s\n' "$plan_output" >&2
        exit 1
    fi
    planned_steps="$(grep -Ec '^\[[0-9]+/[0-9]+\]' <<<"$plan_output" || true)"
}

validate_max_steps() {
    max_steps="${KRITA_IOS_INCREMENTAL_MAX_STEPS:-200}"
    if [[ ! "$max_steps" =~ ^[0-9]+$ ]]; then
        echo "error: KRITA_IOS_INCREMENTAL_MAX_STEPS must be a non-negative integer" >&2
        exit 1
    fi
}

print_plan() {
    plan_lines="$(wc -l <<<"$plan_output" | tr -d ' ')"
    if (( plan_lines <= 80 )); then
        printf '%s\n' "$plan_output"
    else
        sed -n '1,50p' <<<"$plan_output"
        printf '... %s plan lines omitted ...\n' "$((plan_lines - 70))"
        tail -n 20 <<<"$plan_output"
    fi
    printf 'planned build steps: %s\n' "$planned_steps"
    printf 'build tree: %s\n' "$build_dir"
}

build_target() {
    cmake --build "$build_dir" --target "$target" --parallel
    local app="$build_dir/bin/LibrePaint.app"
    local binary="$app/LibrePaint"
    local plist="$app/Info.plist"
    local contract key expected actual
    if [[ ! -x "$binary" || ! -f "$plist" ]]; then
        echo "error: LibrePaint build omitted the expected app bundle or executable" >&2
        exit 1
    fi
    for contract in \
        CFBundleDisplayName=LibrePaint \
        CFBundleExecutable=LibrePaint \
        CFBundleIdentifier=local.librepaint.ipad \
        CFBundleName=LibrePaint; do
        key="${contract%%=*}"
        expected="${contract#*=}"
        actual="$(/usr/bin/plutil -extract "$key" raw -o - "$plist" 2>/dev/null || true)"
        if [[ "$actual" != "$expected" ]]; then
            echo "error: LibrePaint Info.plist $key is '$actual'; expected '$expected'" >&2
            exit 1
        fi
    done
    python3 "$repo_root/packaging/ios/scripts/audit-static-dependency-resources.py" \
        --binary "$binary" \
        --build-ninja "$build_dir/build.ninja"
}

case "$command_name" in
    configure)
        (( $# == 0 )) || usage
        acquire_lock
        configure_tree
        echo "configured incremental build tree: $build_dir"
        ;;
    path)
        (( $# == 0 )) || usage
        echo "$build_dir"
        ;;
    plan)
        (( $# == 0 )) || usage
        acquire_lock
        refresh_tree
        create_plan
        print_plan
        ;;
    build)
        allow_large=0
        if (( $# == 1 )) && [[ "$1" == "--allow-large" ]]; then
            allow_large=1
            shift
        fi
        (( $# == 0 )) || usage

        acquire_lock
        refresh_tree
        create_plan
        validate_max_steps
        if (( planned_steps > max_steps && ! allow_large )); then
            print_plan
            echo "error: refusing an unexpected large rebuild ($planned_steps steps; limit $max_steps)" >&2
            echo "use 'bootstrap' for a new baseline or 'build --allow-large' for an intentional broad change" >&2
            exit 3
        fi
        printf 'building %s planned step(s) in %s\n' "$planned_steps" "$build_dir"
        build_target
        ;;
    bootstrap)
        (( $# == 0 )) || usage
        acquire_lock
        configure_tree
        create_plan
        printf 'building baseline with %s planned step(s) in %s\n' "$planned_steps" "$build_dir"
        build_target
        ;;
    deploy)
        (( $# <= 1 )) || usage
        acquire_lock
        refresh_tree
        create_plan
        validate_max_steps
        if (( planned_steps > max_steps )); then
            print_plan
            echo "error: refusing to deploy after an unexpected large rebuild ($planned_steps steps; limit $max_steps)" >&2
            echo "complete an explicit bootstrap first" >&2
            exit 3
        fi
        build_target
        # The Nix build environment deliberately puts a restricted xcrun shim
        # first. Deployment needs the host xcrun for lipo, vtool and devicectl.
        PATH="/usr/bin:/bin:/usr/sbin:/sbin:$PATH" \
            KRITA_IOS_BUILD_DIR="$build_dir" \
            "$repo_root/packaging/ios/scripts/deploy-altstore.sh" --skip-build "$@"
        ;;
    *)
        usage
        ;;
esac
