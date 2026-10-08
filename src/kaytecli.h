#ifndef KAYTECLI_H
#define KAYTECLI_H

// Command-line mode of the KayteIDE binary, used when it runs as `kayte`
// (the copy bundled at KayteIDE.app/Contents/tools/sdk/usr/bin/kayte). It
// drives the Kayte SDK (github.com/ringsce/kayte-lang) bundled at
// Contents/tools/kayte for .xproj projects:
//
//   kayte build [project] [--config Debug|Release]
//   kayte run   [project] [--config …] [-- program arguments]
//   kayte clean [project] [--config …] [--all]
//
// `project` is a .xproj file or a folder containing one (default: the
// current folder). Any other arguments (--compile, --native, --run, --repl,
// …) are passed straight to the Kayte compiler.
int kayteCliMain(int argc, char *argv[]);

// True when argv[0] names the `kayte` tool rather than the IDE.
bool isKayteCliInvocation(const char *argv0);

#endif // KAYTECLI_H
