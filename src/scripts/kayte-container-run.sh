#!/bin/bash
#
# kayte-container-run — build a Kayte program for Linux and run it in a fresh
# Apple `container` VM (linux/arm64 natively, or linux/amd64 through
# Rosetta 2), then delete every VM, image and temp file the run created.
#
# Nothing is reused between runs: each run copies the bundled Kayte SDK
# (KayteIDE.app/Contents/tools/kayte), rebuilds its Linux builder image from
# scratch with the SDK's own scripts/build-kayte-debian-container.sh
# (Debian + fpc + Lazarus built from source — this takes a while), builds the
# Linux Kayte compiler in it, compiles the program, and runs the result in a
# clean Debian VM.
#
# Usage:
#   kayte-container-run [--arch arm64|amd64] [--build-only|--debug] <program> [-- args]
#   kayte-container-run --clean
#
#   <program>     a .xproj, a folder containing one, or a .kayte / .kjs file
#   --arch        arm64 (default, native) or amd64 (x86-64 via Rosetta 2)
#   --build-only  stop after compiling; the Linux binary is kept in the
#                 project's build/linux-<arch>/ folder
#   --debug       build with debug info (-g -O0) and start gdb on the program
#                 in the VM; gdb reads commands from stdin (the IDE's Debug
#                 Console). Quitting gdb ends the session and cleans up.
#   --clean       remove anything left behind by an interrupted run

set -uo pipefail

# GUI apps (the IDE) start with a minimal PATH.
export PATH="/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"

ARCH="arm64"
BUILD_ONLY=0
DEBUG=0
CLEAN_ONLY=0
PROGRAM=""
PROGRAM_ARGS=()
DEBIAN_VERSION="${DEBIAN_VERSION:-13-slim}"
BASE_IMAGE="debian:${DEBIAN_VERSION}"
PREFIX="kayteide-run"

if [ -t 1 ]; then BOLD=$'\033[1m'; RESET=$'\033[0m'; else BOLD=""; RESET=""; fi
say()  { printf '\n%s==> %s%s\n' "$BOLD" "$*" "$RESET"; }
# The IDE's output panel shows raw escape codes: strip them unless on a terminal.
plain() { if [ -t 1 ]; then cat; else sed -l "s/$(printf '\033')\[[0-9;]*[A-Za-z]//g"; fi; }
info() { printf '    %s\n' "$*"; }
die()  { printf 'kayte-container-run: %s\n' "$*" >&2; exit 1; }

usage() { sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0; }

while [ $# -gt 0 ]; do
    case "$1" in
        --arch)        ARCH="${2:?--arch needs arm64 or amd64}"; shift ;;
        --arch=*)      ARCH="${1#--arch=}" ;;
        --build-only)  BUILD_ONLY=1 ;;
        --debug)       DEBUG=1 ;;
        --clean)       CLEAN_ONLY=1 ;;
        -h|--help)     usage ;;
        --)            shift; PROGRAM_ARGS=("$@"); break ;;
        -*)            die "unknown option: $1 (see --help)" ;;
        *)             [ -z "$PROGRAM" ] && PROGRAM="$1" || die "unexpected argument: $1" ;;
    esac
    shift
done

[ "$DEBUG" = 1 ] && [ "$BUILD_ONLY" = 1 ] && die "--debug and --build-only don't go together"

case "$ARCH" in
    arm64|aarch64) ARCH="arm64" ;;
    amd64|x86_64|x64) ARCH="amd64" ;;
    *) die "unsupported --arch '$ARCH' (use arm64 or amd64)" ;;
esac
PLATFORM="linux/$ARCH"

[ "$(uname -s)" = "Darwin" ] || die "Apple's container tool only runs on macOS"
[ "$(uname -m)" = "arm64" ]  || die "Apple's container tool needs an Apple silicon Mac"

# ─── container CLI and services ──────────────────────────────────────────────
if ! command -v container >/dev/null 2>&1; then
    command -v brew >/dev/null 2>&1 \
        || die "Apple's container CLI is not installed: https://github.com/apple/container/releases"
    say "Installing Apple's container CLI"
    brew install container || die "could not install the container CLI"
fi

SYSTEM_WAS_RUNNING=0
container system status >/dev/null 2>&1 && SYSTEM_WAS_RUNNING=1
start_system() {
    if [ "$SYSTEM_WAS_RUNNING" = 0 ] && ! container system status >/dev/null 2>&1; then
        info "starting container services"
        container system start --enable-kernel-install >/dev/null 2>&1 \
            || container system start >/dev/null 2>&1 \
            || die "could not start the container services (run 'container system start' once in a terminal)"
    fi
}

image_exists() { container image list 2>/dev/null | awk 'NR > 1 { print $1 ":" $2; print $1 }' | grep -qx "$1"; }

# ─── --clean: leftovers from interrupted runs ────────────────────────────────
if [ "$CLEAN_ONLY" = 1 ]; then
    say "Removing leftovers of earlier container runs"
    start_system
    for c in $(container list -a 2>/dev/null | awk 'NR > 1 { print $1 }' | grep -E "^($PREFIX|kayte-debian-build)"); do
        container rm -f "$c" >/dev/null 2>&1 && info "removed container $c"
    done
    for i in $(container image list 2>/dev/null | awk 'NR > 1 { print $1 }' | grep -E "^$PREFIX"); do
        container image rm "$i" >/dev/null 2>&1 && info "removed image $i"
    done
    container builder delete --force >/dev/null 2>&1 && info "removed the builder VM"
    rm -rf "${TMPDIR:-/tmp}"/kayteide-container.* && info "removed temp folders"
    [ "$SYSTEM_WAS_RUNNING" = 0 ] && container system stop >/dev/null 2>&1
    exit 0
fi

# ─── The program ─────────────────────────────────────────────────────────────
[ -n "$PROGRAM" ] || die "no program given (a .xproj, its folder, or a .kayte file)"
[ -e "$PROGRAM" ] || die "not found: $PROGRAM"
PROGRAM="$(cd "$(dirname "$PROGRAM")" && pwd)/$(basename "$PROGRAM")"
[ -d "$PROGRAM" ] && { x="$(ls "$PROGRAM"/*.xproj 2>/dev/null | head -1)"; [ -n "$x" ] || die "no .xproj in $PROGRAM"; PROGRAM="$x"; }

xml() { sed -n "s:.*<$1>[[:space:]]*\\([^<]*[^<[:space:]]\\)[[:space:]]*</$1>.*:\\1:p" "$PROGRAM" | head -1; }
case "$PROGRAM" in
    *.xproj)
        PROJECT_DIR="$(dirname "$PROGRAM")"
        NAME="$(xml Name)"; [ -n "$NAME" ] || NAME="$(basename "$PROGRAM" .xproj)"
        MAIN="$(sed -n 's:.*<File>[[:space:]]*\([^<]*\.\(kayte\|kjs\|kyt\)\)[[:space:]]*</File>.*:\1:p' "$PROGRAM" | head -1)"
        if [ -z "$MAIN" ]; then
            SRC="$(xml SourceDir)"; MAIN="${SRC:-src}/main.kayte"
        fi
        ;;
    *.kayte|*.kjs|*.kyt)
        PROJECT_DIR="$(dirname "$PROGRAM")"
        MAIN="$(basename "$PROGRAM")"
        NAME="${MAIN%.*}"
        ;;
    *) die "not a Kayte program: $PROGRAM" ;;
esac
[ -f "$PROJECT_DIR/$MAIN" ] || die "main source not found: $PROJECT_DIR/$MAIN"
NAME="$(printf '%s' "$NAME" | tr -c 'A-Za-z0-9._-' '_')"

# The Kayte SDK bundled with KayteIDE (this script lives in Contents/tools/bin).
SDK="${KAYTE_SDK:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)/kayte}"
[ -f "$SDK/scripts/build-kayte-debian-container.sh" ] \
    || die "Kayte SDK not found at $SDK (run src/scripts/requirements.sh to bundle it)"

if [ "$ARCH" = "amd64" ] && ! /usr/bin/arch -x86_64 /usr/bin/true 2>/dev/null; then
    die "linux/amd64 runs through Rosetta 2, which is not installed (run: softwareupdate --install-rosetta)"
fi

# ─── Everything this run creates, and its removal ────────────────────────────
RUN_ID="$(date +%s)-$$"
IMAGE="$PREFIX-$ARCH-$RUN_ID"
C_SDK="$PREFIX-sdk-$RUN_ID"
C_COMPILE="$PREFIX-compile-$RUN_ID"
C_APP="$PREFIX-app-$RUN_ID"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/kayteide-container.XXXXXX")"
start_system
BASE_EXISTED=0; image_exists "$BASE_IMAGE" && BASE_EXISTED=1
BUILDER_EXISTED=0
container list -a 2>/dev/null | awk 'NR > 1 { print $1 }' | grep -qx buildkit && BUILDER_EXISTED=1

cleanup() {
    local rc=$?
    trap - EXIT INT TERM
    say "Cleaning up VMs, images and temp files"
    for c in "$C_SDK" "$C_COMPILE" "$C_APP"; do
        container rm -f "$c" >/dev/null 2>&1 && info "removed VM $c"
    done
    container image rm "$IMAGE" >/dev/null 2>&1 && info "removed image $IMAGE"
    if [ "$BASE_EXISTED" = 0 ] && image_exists "$BASE_IMAGE"; then
        container image rm "$BASE_IMAGE" >/dev/null 2>&1 && info "removed image $BASE_IMAGE"
    fi
    if [ "$BUILDER_EXISTED" = 0 ]; then
        container builder delete --force >/dev/null 2>&1 && info "removed the builder VM (and its cache)"
    fi
    rm -rf "$WORK" && info "removed $WORK"
    if [ "$SYSTEM_WAS_RUNNING" = 0 ]; then
        container system stop >/dev/null 2>&1 && info "stopped container services"
    fi
    exit "$rc"
}
trap cleanup EXIT
trap 'exit 130' INT TERM

# Output folder in the project: the Linux binary and the full build log.
OUT_DIR="$PROJECT_DIR/build/linux-$ARCH"
mkdir -p "$OUT_DIR" || die "cannot create $OUT_DIR"
LOG="$OUT_DIR/container-build.log"
: > "$LOG"

# Runs a long, noisy step with its output in $LOG; on failure shows the end.
logged() {
    local what="$1"; shift
    # Only the gdb session reads stdin (the IDE's Debug Console writes there).
    if "$@" >>"$LOG" 2>&1 </dev/null; then return 0; fi
    printf '\n--- last lines of %s ---\n' "$LOG"
    tail -40 "$LOG" | plain
    die "$what failed (full log: $LOG)"
}

info "program:  $PROJECT_DIR/$MAIN ($NAME)"
info "platform: $PLATFORM$([ "$ARCH" = amd64 ] && echo ' (x86-64 through Rosetta 2)')"
info "work dir: $WORK"
info "build log: $LOG"

# ─── 1. Fresh copies of the SDK and the program ──────────────────────────────
say "Copying the Kayte SDK and the program"
mkdir -p "$WORK/out"
rsync -a --exclude .git --exclude /bin/ --exclude /build/ --exclude /lib/ \
      --exclude '*.o' --exclude '*.ppu' "$SDK/" "$WORK/kayte-lang/" || die "could not copy the SDK"
rsync -a --exclude .git --exclude /build/ --exclude /.kayteide/ \
      "$PROJECT_DIR/" "$WORK/project/" || die "could not copy the program"

# The SDK's script builds for linux/arm64 with a fixed container name and a
# cached image; run a copy that targets $PLATFORM, uses this run's names and
# rebuilds from scratch.
SDK_SCRIPT="$WORK/kayte-lang/scripts/build-kayte-debian-container.sh"
sed -i '' \
    -e "s#--platform linux/arm64#--platform $PLATFORM#g" \
    -e "s#container build #container build --no-cache #" \
    -e "s#container run --rm#container run --rm --platform $PLATFORM#g" \
    -e "s#^CONTAINER_NAME=.*#CONTAINER_NAME=\"$C_SDK\"#" \
    "$SDK_SCRIPT"
if [ "$DEBUG" = 1 ]; then
    # gdb in the builder image (the SDK's Dockerfile, written by the script).
    perl -pi -e 'if (/^(\s+)binutils \\\\$/) { $_ .= "$1gdb \\\\\n" }' "$SDK_SCRIPT"
    grep -q '^ *gdb \\\\$' "$SDK_SCRIPT" || die "could not add gdb to the SDK's builder image"
fi

# ─── 2. Builder VM image + Linux Kayte compiler (SDK script) ────────────────
say "Building the Linux builder image from scratch ($PLATFORM) — this takes a while"
logged "building the builder image" \
    env IMAGE_NAME="$IMAGE" DEBIAN_VERSION="$DEBIAN_VERSION" bash "$SDK_SCRIPT" image
info "image $IMAGE ready"

say "Building the Kayte compiler for $PLATFORM"
logged "building the Linux Kayte compiler" \
    env IMAGE_NAME="$IMAGE" DEBIAN_VERSION="$DEBIAN_VERSION" bash "$SDK_SCRIPT" build
[ -x "$WORK/kayte-lang/build/debian/kayte" ] || die "the Linux Kayte compiler was not produced (log: $LOG)"
info "kayte $(file -b "$WORK/kayte-lang/build/debian/kayte" | cut -d, -f1-2)"

# ─── 3. Compile the program in a fresh VM ────────────────────────────────────
say "Compiling $MAIN for $PLATFORM"
MAIN_DIR="$(dirname "$MAIN")"; MAIN_FILE="$(basename "$MAIN")"
# kayte runs the C compiler as KAYTE_CC. The wrapper:
#  - defines _GNU_SOURCE, for SDKs before kayte-lang 37cfa90, whose generated
#    C hid nanosleep / readlink / realpath from the runtime on glibc;
#  - when debugging, swaps -O2 for -O0 and adds -g.
if [ "$DEBUG" = 1 ]; then
    cat > "$WORK/out/kayte-cc" <<'CCEOF'
#!/bin/sh
for a; do shift; [ "$a" = -O2 ] && a=-O0; set -- "$@" "$a"; done
exec cc -D_GNU_SOURCE -g "$@"
CCEOF
else
    printf '#!/bin/sh\nexec cc -D_GNU_SOURCE "$@"\n' > "$WORK/out/kayte-cc"
fi
chmod +x "$WORK/out/kayte-cc"
KEEP_C=""; [ "$DEBUG" = 1 ] && KEEP_C="--keep-c"   # gdb lists and steps the generated C
HOST_MAIN="$PROJECT_DIR/$MAIN"
# Errors come out as "Parser Error: msg at 2:25"; name the file (host path) so
# the IDE lists them under Problems.
container run --rm --platform "$PLATFORM" --name "$C_COMPILE" \
    -v "$WORK:/work" -w "/work/project/$MAIN_DIR" "$IMAGE" \
    -lc "export LD_LIBRARY_PATH=/work/kayte-lang/build/debian KAYTE_CC=/work/out/kayte-cc; \
         /work/kayte-lang/build/debian/kayte --native '$MAIN_FILE' -o '/work/out/$NAME' $KEEP_C" 2>&1 </dev/null \
  | plain | grep -v -E '^\[[0-9]+/[0-9]+\] ' | sed -E \
      -e '/^(Kayte Language Runtime Environment|=+)$/d' \
      -e "s#^([A-Za-z]+ )?Error: (.+) at ([0-9]+):([0-9]+)( \\(.*\\))?\$#$HOST_MAIN:\\3:\\4: error: \\2#" \
      -e "s#^([A-Za-z]+ )?Error: (.+) at line ([0-9]+)\$#$HOST_MAIN:\\3: error: \\2#"
[ "${PIPESTATUS[0]}" = 0 ] && [ -x "$WORK/out/$NAME" ] || die "compiling $MAIN failed"

cp -f "$WORK/out/$NAME" "$OUT_DIR/$NAME"
[ -f "$WORK/out/$NAME.c" ] && cp -f "$WORK/out/$NAME.c" "$OUT_DIR/$NAME.c"
info "Linux binary: $OUT_DIR/$NAME ($(file -b "$OUT_DIR/$NAME" | cut -d, -f1-2))"

[ "$BUILD_ONLY" = 1 ] && exit 0

# ─── 4a. Debug: gdb in a new VM, driven through stdin ───────────────────────
if [ "$DEBUG" = 1 ]; then
    say "Debugging $NAME with gdb in a new $PLATFORM VM$([ "$ARCH" = amd64 ] && echo ' (Rosetta 2)')"
    info "Type gdb commands in the Debug Console, e.g.:"
    info "  break main   run   next   step   bt   info locals   print <expr>   continue   quit"
    info "Source: the C kayte generated ($NAME.c, one label per instruction) and the Kayte runtime."
    info "quit ends the session; the VM and everything else are then removed."
    ARGS_Q=""
    for a in ${PROGRAM_ARGS[@]+"${PROGRAM_ARGS[@]}"}; do ARGS_Q="$ARGS_Q $(printf '%q' "$a")"; done
    GDB_OPTS="-q -nx -ex 'set pagination off' -ex 'set confirm off' -ex 'set width 0' -ex 'set listsize 20'"
    if [ "$ARCH" = amd64 ]; then
        # Rosetta 2 doesn't support ptrace on the x86-64 processes it runs, so
        # gdb can't start the program itself. Rosetta's own debug stub can:
        # with ROSETTA_DEBUGSERVER_PORT set, the program waits at its first
        # instruction for a gdb remote connection.
        info "amd64: the program is already started and paused — use 'continue' (not 'run')."
        VM_CMD="ROSETTA_DEBUGSERVER_PORT=1234 '/work/out/$NAME'$ARGS_Q & \
                sleep 2; \
                exec gdb $GDB_OPTS -ex 'target remote 127.0.0.1:1234' '/work/out/$NAME'"
    else
        VM_CMD="exec gdb $GDB_OPTS --args '/work/out/$NAME'$ARGS_Q"
    fi
    # Same paths as at compile time, so gdb finds the sources.
    container run -i --rm --platform "$PLATFORM" --name "$C_APP" \
        -v "$WORK:/work" -w /work/out "$IMAGE" \
        -lc "echo \"[vm] \$(uname -srm)\"; $VM_CMD" 2>&1 \
      | grep --line-buffered -v -E '^\[[0-9]+/[0-9]+\] '
    DBG_RC=${PIPESTATUS[0]}
    printf '\n[debug session ended with code %d]\n' "$DBG_RC"
    exit "$DBG_RC"
fi

# ─── 4. Run it in a clean Debian VM ──────────────────────────────────────────
say "Running $NAME in a new $PLATFORM VM$([ "$ARCH" = amd64 ] && echo ' (Rosetta 2)')"
container run --rm --platform "$PLATFORM" --name "$C_APP" \
    -v "$WORK/out:/app" -w /app "$BASE_IMAGE" \
    /bin/sh -c 'echo "[vm] $(uname -srm) - $(. /etc/os-release && echo "$PRETTY_NAME")"; echo; exec "/app/$0" "$@"' \
    "$NAME" ${PROGRAM_ARGS[@]+"${PROGRAM_ARGS[@]}"} 2>&1 \
  | plain | grep -v -E '^\[[0-9]+/[0-9]+\] '
APP_RC=${PIPESTATUS[0]}
printf '\n[%s exited with code %d]\n' "$NAME" "$APP_RC"
exit "$APP_RC"
