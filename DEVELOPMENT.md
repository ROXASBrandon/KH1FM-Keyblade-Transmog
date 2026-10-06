# Development and release checks

Use Python 3 and `ziglang==0.16.0`. Run `python native/build.py` to test the
cosmetic logic and build the Windows x64 DLL. Windows builds also run the
native sound-hook integration test against an isolated synthetic image; the
test never opens or patches a game process.

To inspect the supported executable locally, install `pefile`, then run
`python tests/verify_transmog_executable.py <KH1.exe> <extracted-KH1-assets>`.
Executable signatures, two sound-only formatting sites, item metadata, and
all 18 weapon assets must match. These proprietary files are not distributed.

Before updating downloads, run `python tools/package.py`. The package uses
an explicit file list, checks for private paths, runs ZIP integrity checks,
and writes the SHA-256 checksum alongside the preview ZIP.

## Gameplay status

- Confirmed: Q appearance cycling and room-transition persistence.
- Live inspection: equipped Jungle King remained equipped with another model;
  all weapon parameter bytes after the filename matched the original checksum.
- Automated: all 324 equipped/appearance sound-name combinations, unchanged
  sound IDs and combat fields, restoration, input edge detection, and Win64
  call/relay behavior.
- Pending: clean restart on 0.1.2, restored hit audio, Shift+Q, other equipped
  weapons, death/continue, fresh OpenKH install, removal, and another PC.

Keep this build labeled preview until those gameplay checks pass. A compiled
or synthetic-test result does not establish audible in-game behavior.
