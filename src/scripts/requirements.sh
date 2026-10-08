#!/bin/bash
#
# requirements.sh — fetch and bundle KayteIDE's external toolchain (macOS).
#
# Installs/checks the host prerequisites (Rosetta 2, Xcode Command Line Tools,
# Homebrew) and then places a self-contained toolchain inside the built app:
#
#   KayteIDE.app/Contents/tools/
#     bin/        wrappers: fpc, lazbuild, lazarus, qemu-* (use these)
#     fpc/        Free Pascal Compiler (extracted from the official .pkg)
#     lazarus/    Lazarus IDE (portable zip, config points at ../bin/fpc)
#     qemu/       QEMU binaries + dylibs + firmware (relocated from Homebrew)
#     kayte/      Kayte language SDK (github.com/ringsce/kayte-lang), built
#                 in place with the bundled Lazarus; bin/kayte links to it
#     env.sh      `source` it to put the bundled tools on PATH
#     VERSIONS.txt
#
# Usage: src/scripts/requirements.sh [options]
#   --rosetta              check for Rosetta 2 and install it if missing
#   --app PATH             KayteIDE.app to fill (default: build/bin/KayteIDE.app)
#   --lazarus-version V    Lazarus release on SourceForge (default: 4.8)
#   --qemu-targets "a b"   qemu-system-* targets to bundle
#                          (default: "aarch64 x86_64 i386 arm riscv64")
#   --keep-i386            keep the i386-darwin compiler/units (dropped by default)
#   --kayte-ref REF        kayte-lang branch or tag to build (default: its default branch)
#   --skip-xcode | --skip-fpc | --skip-lazarus | --skip-qemu | --skip-kayte
#   --non-interactive      no terminal (KayteIDE's first-run setup): admin
#                          password via the macOS dialog, and QEMU is skipped
#                          with a warning when Homebrew is not installed
#   --clean-cache          delete cached downloads first
#   -h, --help
#
# Downloads are cached in ~/Library/Caches/KayteIDE/downloads.
# Everything is resolved relative to the bundle (QEMU dylibs via
# @executable_path), so the .app can be moved after running this.

set -euo pipefail

# ─── Defaults ────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
# GUI apps (KayteIDE's first-run setup) start with a minimal PATH.
export PATH="/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"

# Installed in KayteIDE.app/Contents/tools/scripts: fill that app.
# From the source tree: the app the build produced.
case "$SCRIPT_DIR" in
    */Contents/tools/scripts) APP_PATH="$(cd "$SCRIPT_DIR/../../.." && pwd)" ;;
    *)                        APP_PATH="$REPO_ROOT/build/bin/KayteIDE.app" ;;
esac
NON_INTERACTIVE=0
LAZARUS_VERSION="4.8"
KAYTE_REPO="https://github.com/ringsce/kayte-lang.git"
KAYTE_REF=""
QEMU_TARGETS="aarch64 x86_64 i386 arm riscv64"
WANT_ROSETTA=0
KEEP_I386=0
SKIP_XCODE=0
SKIP_FPC=0
SKIP_LAZARUS=0
SKIP_QEMU=0
SKIP_KAYTE=0
CLEAN_CACHE=0

CACHE_DIR="$HOME/Library/Caches/KayteIDE/downloads"
SF_BASE="https://downloads.sourceforge.net/project/lazarus"
SF_RSS="https://sourceforge.net/projects/lazarus/rss"

# ─── Helpers ─────────────────────────────────────────────────────────────────
if [ -t 1 ]; then
    C_B=$'\033[1m'; C_G=$'\033[32m'; C_Y=$'\033[33m'; C_R=$'\033[31m'; C_0=$'\033[0m'
else
    C_B=""; C_G=""; C_Y=""; C_R=""; C_0=""
fi
step() { printf '\n%s==> %s%s\n' "$C_B" "$*" "$C_0"; }
info() { printf '    %s\n' "$*"; }
ok()   { printf '    %s✔ %s%s\n' "$C_G" "$*" "$C_0"; }
warn() { printf '    %s! %s%s\n' "$C_Y" "$*" "$C_0" >&2; }
die()  { printf '%s✘ %s%s\n' "$C_R" "$*" "$C_0" >&2; exit 1; }

usage() { sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0; }

urlencode_path() { printf '%s' "$1" | sed 's/ /%20/g'; }

# download URL DEST — cached, resumable, retried
download() {
    local url="$1" dest="$2"
    if [ -s "$dest" ]; then
        info "cached: $(basename "$dest")"
        return 0
    fi
    info "downloading $(basename "$dest")"
    curl -fL --retry 3 --retry-delay 2 --progress-bar -C - -o "$dest.part" "$url" \
        || die "download failed: $url"
    mv "$dest.part" "$dest"
}

WORK_DIR=""
MOUNT_POINT=""
cleanup() {
    if [ -n "$MOUNT_POINT" ] && mount | grep -q " on $MOUNT_POINT "; then
        hdiutil detach -quiet "$MOUNT_POINT" || true
    fi
    [ -n "$WORK_DIR" ] && rm -rf "$WORK_DIR"
}
trap cleanup EXIT
trap 'exit 143' TERM INT HUP   # cancelled (e.g. from KayteIDE): still clean up

# ─── Arguments ───────────────────────────────────────────────────────────────
while [ $# -gt 0 ]; do
    case "$1" in
        --rosetta)          WANT_ROSETTA=1 ;;
        --app)              APP_PATH="${2:?--app needs a path}"; shift ;;
        --lazarus-version)  LAZARUS_VERSION="${2:?--lazarus-version needs a value}"; shift ;;
        --qemu-targets)     QEMU_TARGETS="${2:?--qemu-targets needs a value}"; shift ;;
        --keep-i386)        KEEP_I386=1 ;;
        --skip-xcode)       SKIP_XCODE=1 ;;
        --skip-fpc)         SKIP_FPC=1 ;;
        --skip-lazarus)     SKIP_LAZARUS=1 ;;
        --skip-qemu)        SKIP_QEMU=1 ;;
        --skip-kayte)       SKIP_KAYTE=1 ;;
        --non-interactive)  NON_INTERACTIVE=1 ;;
        --kayte-ref)        KAYTE_REF="${2:?--kayte-ref needs a branch or tag}"; shift ;;
        --clean-cache)      CLEAN_CACHE=1 ;;
        -h|--help)          usage ;;
        *)                  die "unknown option: $1 (see --help)" ;;
    esac
    shift
done

[ "$(uname -s)" = "Darwin" ] || die "this script only supports macOS"

HOST_ARCH="$(uname -m)"   # arm64 | x86_64
case "$HOST_ARCH" in
    arm64)  SF_DIR="Lazarus macOS aarch64"; LAZ_ARCH="aarch64" ;;
    x86_64) SF_DIR="Lazarus macOS x86-64";  LAZ_ARCH="x86_64" ;;
    *)      die "unsupported architecture: $HOST_ARCH" ;;
esac

[ -d "$APP_PATH/Contents" ] || die "app bundle not found: $APP_PATH (build KayteIDE first or pass --app)"
APP_PATH="$(cd "$APP_PATH" && pwd)"
TOOLS_DIR="$APP_PATH/Contents/tools"

[ "$CLEAN_CACHE" = 1 ] && rm -rf "$CACHE_DIR"
mkdir -p "$CACHE_DIR" "$TOOLS_DIR/bin"
WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/kayte-req.XXXXXX")"

echo "${C_B}KayteIDE requirements${C_0}"
info "host:    macOS $(sw_vers -productVersion) ($HOST_ARCH)"
info "app:     $APP_PATH"
info "tools:   $TOOLS_DIR"

# ─── Rosetta 2 ───────────────────────────────────────────────────────────────
if [ "$WANT_ROSETTA" = 1 ]; then
    step "Rosetta 2"
    if [ "$HOST_ARCH" != "arm64" ]; then
        ok "not needed on Intel Macs"
    elif /usr/bin/arch -x86_64 /usr/bin/true 2>/dev/null; then
        ok "already installed"
    else
        info "installing (requires administrator password)"
        if [ "$NON_INTERACTIVE" = 1 ]; then
            # No terminal for sudo: macOS asks for the password in a dialog.
            /usr/bin/osascript -e 'do shell script "/usr/sbin/softwareupdate --install-rosetta --agree-to-license" with administrator privileges' >/dev/null \
                || die "Rosetta 2 installation failed or was cancelled"
        else
            sudo /usr/sbin/softwareupdate --install-rosetta --agree-to-license \
                || die "Rosetta 2 installation failed"
        fi
        /usr/bin/arch -x86_64 /usr/bin/true 2>/dev/null || die "Rosetta 2 still not usable"
        ok "installed"
    fi
fi

# ─── Xcode Command Line Tools ────────────────────────────────────────────────
clt_ready() { xcode-select -p >/dev/null 2>&1 && xcrun --show-sdk-path >/dev/null 2>&1; }

if [ "$SKIP_XCODE" = 0 ]; then
    step "Xcode Command Line Tools"
    if clt_ready; then
        ok "found at $(xcode-select -p)"
    else
        info "launching the installer — accept the dialog to continue"
        xcode-select --install 2>/dev/null || true
        waited=0
        until clt_ready; do
            [ "$waited" -ge 3600 ] && die "timed out waiting for the Command Line Tools"
            sleep 10; waited=$((waited + 10))
        done
        ok "installed at $(xcode-select -p)"
    fi
fi

# ─── Discover release file names on SourceForge ──────────────────────────────
SF_RELEASE_PATH="/$SF_DIR/Lazarus $LAZARUS_VERSION"
LAZ_ZIP="lazarus-darwin-$LAZ_ARCH-$LAZARUS_VERSION.zip"
FPC_DMG=""

if [ "$SKIP_FPC" = 0 ] || [ "$SKIP_LAZARUS" = 0 ]; then
    step "Locating Lazarus $LAZARUS_VERSION on SourceForge"
    rss="$(curl -fsL "$SF_RSS?path=$(urlencode_path "$SF_RELEASE_PATH")&limit=20" || true)"
    FPC_DMG="$(printf '%s' "$rss" | grep -o 'fpc-[^/<]*-macosx\.dmg' | head -1 || true)"
    if [ -z "$FPC_DMG" ]; then
        # Fall back to whatever is already cached for this release.
        FPC_DMG="$(cd "$CACHE_DIR" && ls fpc-*-macosx.dmg 2>/dev/null | head -1 || true)"
    fi
    [ -n "$FPC_DMG" ] || die "could not find an FPC dmg for Lazarus $LAZARUS_VERSION ($SF_RELEASE_PATH)"
    if [ -n "$rss" ] && ! printf '%s' "$rss" | grep -q "$LAZ_ZIP"; then
        die "Lazarus $LAZARUS_VERSION has no $LAZ_ZIP on SourceForge"
    fi
    ok "fpc:     $FPC_DMG"
    ok "lazarus: $LAZ_ZIP"
fi
SF_URL="$SF_BASE$(urlencode_path "$SF_RELEASE_PATH")"

# ─── Free Pascal Compiler ────────────────────────────────────────────────────
if [ "$SKIP_FPC" = 0 ]; then
    step "Free Pascal Compiler"
    download "$SF_URL/$FPC_DMG" "$CACHE_DIR/$FPC_DMG"

    MOUNT_POINT="$WORK_DIR/fpc-mnt"
    mkdir -p "$MOUNT_POINT"
    hdiutil attach -quiet -nobrowse -readonly -mountpoint "$MOUNT_POINT" "$CACHE_DIR/$FPC_DMG" \
        || die "could not mount $FPC_DMG"
    pkg="$(ls "$MOUNT_POINT"/*.pkg 2>/dev/null | head -1 || true)"
    [ -n "$pkg" ] || die "no .pkg found inside $FPC_DMG"

    info "extracting $(basename "$pkg")"
    pkgutil --expand-full "$pkg" "$WORK_DIR/fpc-pkg" >/dev/null
    hdiutil detach -quiet "$MOUNT_POINT"; MOUNT_POINT=""

    payload="$(find "$WORK_DIR/fpc-pkg" -type d -path '*/Payload/usr/local' | head -1)"
    [ -n "$payload" ] && [ -x "$payload/bin/fpc" ] || die "unexpected FPC package layout"

    rm -rf "$TOOLS_DIR/fpc"
    ditto "$payload" "$TOOLS_DIR/fpc"
    FPC_VERSION="$("$TOOLS_DIR/fpc/bin/fpc" -iV)"

    if [ "$KEEP_I386" = 0 ]; then
        rm -f  "$TOOLS_DIR/fpc/bin/ppc386" "$TOOLS_DIR/fpc/lib/fpc/$FPC_VERSION/ppc386"
        rm -rf "$TOOLS_DIR/fpc/lib/fpc/$FPC_VERSION/units/i386-darwin"
    fi
    xattr -cr "$TOOLS_DIR/fpc"

    # Relocatable driver: ignores any system/user fpc.cfg and uses one that
    # points at the units shipped next to it, so the app can be moved freely.
    # The paths must come from a cfg file — FPC does not use command-line -Fu
    # paths when resolving units referenced from other .ppu files.
    cat > "$TOOLS_DIR/bin/fpc" <<'EOF'
#!/bin/bash
# KayteIDE bundled Free Pascal driver (generated by requirements.sh)
TOOLS="$(cd "$(dirname "$0")/.." && pwd -P)"
FPCDIR="$TOOLS/fpc"
SDK="$(xcrun --show-sdk-path 2>/dev/null)"
CFG_REV=2   # bump when the cfg contents below change
CFG="${TMPDIR:-/tmp}/kayteide-fpc-$(printf '%s|%s|%s' "$CFG_REV" "$TOOLS" "$SDK" | cksum | cut -d' ' -f1).cfg"
if [ ! -s "$CFG" ]; then
    {
        echo "-Fu$FPCDIR/lib/fpc/\$fpcversion/units/\$fpctarget/*"
        echo "-Fu$FPCDIR/lib/fpc/\$fpcversion/units/\$fpctarget/rtl"
        # Apple's current ld rejects FPC 3.2's Objective-C method lists
        # ("malformed method list atom") unless targeting an older macOS.
        # Projects can still override this with their own -WM.
        echo "-WM10.15"
        if [ -n "$SDK" ]; then
            echo "-XR$SDK"
            echo "-Fl$SDK/usr/lib"
        fi
    } > "$CFG.$$" && mv "$CFG.$$" "$CFG"
fi
# -WM10.15 makes ld warn on every link that the RTL objects target macOS
# 11; drop just that warning and keep FPC's exit status.
set -o pipefail
"$FPCDIR/bin/fpc" -n "@$CFG" "$@" 2>&1 | sed -l "/was built for newer 'macOS' version/d"
EOF
    chmod +x "$TOOLS_DIR/bin/fpc"

    # Smoke test
    mkdir -p "$WORK_DIR/fpc-test"
    printf "program t; uses SysUtils; begin WriteLn('ok'); end.\n" > "$WORK_DIR/fpc-test/t.pas"
    if (cd "$WORK_DIR/fpc-test" && "$TOOLS_DIR/bin/fpc" -v0 t.pas >/dev/null 2>&1 && [ "$(./t)" = ok ]); then
        ok "FPC $FPC_VERSION installed and compiles"
    else
        die "FPC $FPC_VERSION installed but the test program failed to build"
    fi
fi

# ─── Lazarus ─────────────────────────────────────────────────────────────────
if [ "$SKIP_LAZARUS" = 0 ]; then
    step "Lazarus $LAZARUS_VERSION"
    [ -x "$TOOLS_DIR/bin/fpc" ] || warn "no bundled FPC — Lazarus will not be able to compile"
    download "$SF_URL/$LAZ_ZIP" "$CACHE_DIR/$LAZ_ZIP"

    info "extracting"
    # Lazarus' README: clear quarantine before unzipping.
    xattr -c "$CACHE_DIR/$LAZ_ZIP" 2>/dev/null || true
    ditto -x -k "$CACHE_DIR/$LAZ_ZIP" "$WORK_DIR/laz" || die "corrupt archive: $LAZ_ZIP (try --clean-cache)"
    [ -x "$WORK_DIR/laz/lazarus/lazbuild" ] || die "unexpected Lazarus archive layout"

    rm -rf "$TOOLS_DIR/lazarus"
    ditto "$WORK_DIR/laz/lazarus" "$TOOLS_DIR/lazarus"
    xattr -cr "$TOOLS_DIR/lazarus"

    # lazarus.cfg ships as "--pcp=config --lazarusdir=." (portable mode), so
    # the config below is found relative to the install; paths use macros.
    cat > "$TOOLS_DIR/lazarus/config/environmentoptions.xml" <<EOF
<?xml version="1.0"?>
<CONFIG>
  <EnvironmentOptions>
    <Version Value="110" Lazarus="$LAZARUS_VERSION"/>
    <LazarusDirectory Value="\$(LazarusDir)"/>
    <CompilerFilename Value="\$(LazarusDir)/../bin/fpc"/>
    <FPCSourceDirectory Value="\$(LazarusDir)/fpc/\$(FPCVer)/sources/"/>
    <MakeFilename Value="/usr/bin/make"/>
    <TestBuildDirectory Value="\$ENV(TMPDIR)"/>
    <Debugger Class="TFpLldbDebugger"/>
    <DebuggerFilename Value="/usr/bin/lldb"/>
  </EnvironmentOptions>
</CONFIG>
EOF

    cat > "$TOOLS_DIR/bin/lazbuild" <<'EOF'
#!/bin/sh
# KayteIDE bundled lazbuild (generated by requirements.sh)
TOOLS="$(cd "$(dirname "$0")/.." && pwd -P)"
exec "$TOOLS/lazarus/lazbuild" \
    --lazarusdir="$TOOLS/lazarus" \
    --pcp="$TOOLS/lazarus/config" \
    --compiler="$TOOLS/bin/fpc" "$@"
EOF
    cat > "$TOOLS_DIR/bin/lazarus" <<'EOF'
#!/bin/sh
# KayteIDE bundled Lazarus IDE launcher (generated by requirements.sh)
TOOLS="$(cd "$(dirname "$0")/.." && pwd -P)"
exec open -a "$TOOLS/lazarus/lazarus.app" --args "$@"
EOF
    chmod +x "$TOOLS_DIR/bin/lazbuild" "$TOOLS_DIR/bin/lazarus"

    if "$TOOLS_DIR/bin/lazbuild" --version >/dev/null 2>&1; then
        ok "Lazarus $("$TOOLS_DIR/bin/lazbuild" --version 2>/dev/null | tail -1) installed"
    else
        warn "Lazarus installed but 'lazbuild --version' failed"
    fi
fi

# ─── Kayte SDK ───────────────────────────────────────────────────────────────
# The Kayte compiler/VM is a Lazarus project: clone it into the bundle and
# build it there with the bundled FPC + lazbuild (and clang for its native
# Mach-O backend object). Re-running updates the checkout and rebuilds.
if [ "$SKIP_KAYTE" = 0 ]; then
    step "Kayte SDK"
    [ -x "$TOOLS_DIR/bin/lazbuild" ] || die "the Kayte SDK is built with the bundled Lazarus — install it first (don't pass --skip-lazarus on a fresh bundle)"
    command -v git   >/dev/null 2>&1 || die "git not found (it comes with the Xcode Command Line Tools)"
    command -v clang >/dev/null 2>&1 || die "clang not found (install the Xcode Command Line Tools)"

    KDIR="$TOOLS_DIR/kayte"
    if [ -d "$KDIR/.git" ]; then
        info "updating $(git -C "$KDIR" remote get-url origin)"
        git -C "$KDIR" fetch --quiet --depth 1 origin "${KAYTE_REF:-HEAD}" \
            || die "could not fetch ${KAYTE_REF:-the default branch} of $KAYTE_REPO"
        # The bundled checkout is managed by this script: local edits are discarded.
        git -C "$KDIR" reset --quiet --hard FETCH_HEAD
    else
        info "cloning $KAYTE_REPO${KAYTE_REF:+ ($KAYTE_REF)}"
        rm -rf "$KDIR"
        git clone --quiet --depth 1 ${KAYTE_REF:+--branch "$KAYTE_REF"} "$KAYTE_REPO" "$KDIR" \
            || die "could not clone $KAYTE_REPO"
    fi
    KAYTE_COMMIT="$(git -C "$KDIR" describe --tags --always 2>/dev/null || git -C "$KDIR" rev-parse --short HEAD)"

    info "building $KAYTE_COMMIT"
    if [ "$HOST_ARCH" = "arm64" ]; then
        # source/kaytearm64.pas statically links this object on Apple Silicon.
        clang -c -O2 -std=c11 "$KDIR/source/kayte_arm64_emit.c" -o "$KDIR/source/kayte_arm64_emit.o" \
            || die "could not compile the Kayte native backend (kayte_arm64_emit.c)"
    fi
    if ! (cd "$KDIR/source" && "$TOOLS_DIR/bin/lazbuild" kayte.lpi) >"$WORK_DIR/kayte-build.log" 2>&1 \
       || [ ! -x "$KDIR/bin/kayte" ]; then
        tail -30 "$WORK_DIR/kayte-build.log" >&2
        die "building the Kayte SDK failed"
    fi
    ln -sf "../kayte/bin/kayte" "$TOOLS_DIR/bin/kayte"

    # Smoke test: native compile (via cc) and run.
    mkdir -p "$WORK_DIR/kayte-test"
    printf 'PRINT "ok"\n' > "$WORK_DIR/kayte-test/t.kayte"
    if (cd "$WORK_DIR/kayte-test" && "$KDIR/bin/kayte" --native t.kayte -o t >/dev/null 2>&1 \
        && [ "$(./t)" = ok ]); then
        ok "Kayte $KAYTE_COMMIT built and compiles"
    else
        die "Kayte $KAYTE_COMMIT built but could not compile a test program"
    fi
fi

# ─── Homebrew (needed for QEMU) ──────────────────────────────────────────────
ensure_brew() {
    if command -v brew >/dev/null 2>&1; then return 0; fi
    for b in /opt/homebrew/bin/brew /usr/local/bin/brew; do
        if [ -x "$b" ]; then eval "$("$b" shellenv)"; return 0; fi
    done
    if [ "$NON_INTERACTIVE" = 1 ]; then
        # Homebrew's installer needs a terminal (sudo, prompts).
        return 1
    fi
    info "Homebrew not found — installing it"
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" \
        || die "Homebrew installation failed"
    for b in /opt/homebrew/bin/brew /usr/local/bin/brew; do
        if [ -x "$b" ]; then eval "$("$b" shellenv)"; return 0; fi
    done
    die "Homebrew installed but brew is not on PATH"
}

# ─── QEMU ────────────────────────────────────────────────────────────────────
if [ "$SKIP_QEMU" = 0 ]; then
    step "QEMU"
    if ! ensure_brew; then
        warn "Homebrew is not installed, so QEMU was skipped. Install Homebrew"
        warn "(https://brew.sh), then use Tools ▸ Install / Update Toolchain… again."
        SKIP_QEMU=1
    fi
fi
if [ "$SKIP_QEMU" = 0 ]; then
    for f in qemu dylibbundler; do
        if brew list --formula "$f" >/dev/null 2>&1; then
            ok "brew: $f already installed"
        else
            info "brew install $f"
            brew install "$f"
        fi
    done

    QSRC="$(brew --prefix qemu)"
    QDST="$TOOLS_DIR/qemu"
    rm -rf "$QDST"
    mkdir -p "$QDST/bin" "$QDST/lib" "$QDST/share"

    bins="qemu-img"
    for t in $QEMU_TARGETS; do
        [ -x "$QSRC/bin/qemu-system-$t" ] || die "qemu-system-$t not provided by Homebrew's qemu"
        bins="$bins qemu-system-$t"
    done

    fix_args=""
    for b in $bins; do
        cp "$QSRC/bin/$b" "$QDST/bin/$b"
        chmod u+w "$QDST/bin/$b"
        fix_args="$fix_args -x $QDST/bin/$b"
    done

    info "relocating dylibs"
    # shellcheck disable=SC2086
    dylibbundler -cd -of -b $fix_args \
        -d "$QDST/lib" -p "@executable_path/../lib/" \
        -s "$(brew --prefix)/lib" >"$WORK_DIR/dylibbundler.log" 2>&1 \
        || { cat "$WORK_DIR/dylibbundler.log" >&2; die "dylibbundler failed"; }

    # Rewriting load commands breaks the signature; re-sign ad hoc and keep
    # the original entitlements (com.apple.security.hypervisor for HVF).
    codesign --force -s - "$QDST"/lib/*.dylib >/dev/null 2>&1
    for b in $bins; do
        ent="$WORK_DIR/$b.entitlements"
        codesign -d --entitlements :- "$QSRC/bin/$b" > "$ent" 2>/dev/null || true
        if [ -s "$ent" ]; then
            codesign --force -s - --entitlements "$ent" "$QDST/bin/$b" >/dev/null 2>&1
        else
            codesign --force -s - "$QDST/bin/$b" >/dev/null 2>&1
        fi
        ln -sf "../qemu/bin/$b" "$TOOLS_DIR/bin/$b"
    done

    # QEMU looks for firmware in <bindir>/../share/qemu.
    info "copying firmware"
    cp -RL "$QSRC/share/qemu" "$QDST/share/qemu"
    xattr -cr "$QDST"

    QEMU_VERSION="$("$QDST/bin/qemu-img" --version 2>/dev/null | head -1 | awk '{print $3}')"
    for b in $bins; do
        "$QDST/bin/$b" --version >/dev/null 2>&1 || die "bundled $b does not run"
    done
    ok "QEMU $QEMU_VERSION bundled ($bins)"
fi

# ─── env.sh + manifest ───────────────────────────────────────────────────────
step "Finishing"
cat > "$TOOLS_DIR/env.sh" <<'EOF'
# Source this file to use KayteIDE's bundled toolchain:
#   source /path/to/KayteIDE.app/Contents/tools/env.sh
KAYTE_TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]:-${(%):-%x}}")" && pwd -P)"
export KAYTE_TOOLS
export PATH="$KAYTE_TOOLS/bin:$KAYTE_TOOLS/sdk/usr/bin:$PATH"
EOF

{
    echo "generated: $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    echo "host:      macOS $(sw_vers -productVersion) $HOST_ARCH"
    [ -x "$TOOLS_DIR/fpc/bin/fpc" ]        && echo "fpc:       $("$TOOLS_DIR/fpc/bin/fpc" -iV)"
    [ -x "$TOOLS_DIR/lazarus/lazbuild" ]   && echo "lazarus:   $LAZARUS_VERSION"
    [ -x "$TOOLS_DIR/qemu/bin/qemu-img" ]  && echo "qemu:      $("$TOOLS_DIR/qemu/bin/qemu-img" --version | head -1 | awk '{print $3}')"
    [ -d "$TOOLS_DIR/kayte/.git" ]         && echo "kayte:     $(git -C "$TOOLS_DIR/kayte" describe --tags --always 2>/dev/null)"
    true
} > "$TOOLS_DIR/VERSIONS.txt"

ok "toolchain ready in $TOOLS_DIR ($(du -sh "$TOOLS_DIR" | cut -f1))"
info "use it from a shell with:  source \"$TOOLS_DIR/env.sh\""
if codesign -v "$APP_PATH" >/dev/null 2>&1; then :; else
    info "note: the app's code signature is no longer valid; re-sign before distributing"
fi
