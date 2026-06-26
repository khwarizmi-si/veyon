# ffmpeg.exe (for the Screen Recorder add-on)

The Screen Recorder add-on pipes raw frames to an **ffmpeg** process. The Windows
add-on installer bundles this binary next to the executables.

## What to put here

Place a **Windows LGPL build** of ffmpeg here:

```
3rdparty/ffmpeg/ffmpeg.exe
```

- Use an **LGPL** (not GPL) build to match Khwarizmi/Veyon's licensing. Good
  sources: the "shared/LGPL" builds from gyan.dev or BtbN/FFmpeg-Builds.
- Match the architecture you ship (x64).
- Per the LGPL, ship the corresponding ffmpeg source offer alongside your release.

This file is intentionally **not committed** (it is a large third-party binary).
`make create-addon-installers` copies it into the Screen Recorder installer; if it
is missing, only the Screen Recorder installer is skipped and the others still build.
