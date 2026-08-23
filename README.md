# Capture Detector

![Example](https://github.com/AsoiX-exe/capture-detector/blob/master/blockscreen.png)

A small tool that shows which of your open windows are **hidden from screenshots
and screen recording**.

![platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-2b2b2b?style=flat-square)
![language](https://img.shields.io/badge/language-C-00599C?style=flat-square)
![license](https://img.shields.io/badge/license-MIT-2b2b2b?style=flat-square)

---

## What it does

Some programs can hide their window from screen capture. When you take a screenshot
or record/share your screen, that window turns into a **black box** or vanishes —
even though you still see it on your monitor.

Video apps with DRM, banking apps and secure viewers do this on purpose. Some
**malware** does it too, to stay out of recordings.

`capture-detector` checks every open window and shows you the hidden ones in red at
the top. It only looks — it never changes anything.

```
  capture-detector  screen capture protection scanner
  coded by AsoiX

  !!  1 window(s) HIDDEN from screen capture  !!
  won't show up in screenshots or screen recording

    HIDDEN   DoNotCapture.exe        MONITOR    pid 15940   <no title>

  other visible windows
             steam.exe               NONE       pid 11344   Steam
             chrome.exe              NONE       pid 14088   YouTube - Google Chrome

  17 scanned   1 hidden
```

## Run it

Download the `.exe` from [Releases](../../releases) and just **double-click** it.
The window stays open and waits for **Enter** so you can read the result.

Options (if you run it from a terminal):

```
capture-detector -p    only show hidden windows
capture-detector -a    also show windows with no title
```

## Build it yourself

No libraries needed, just a C compiler.

```sh
# Windows (MinGW)
gcc source/capture-detector.c -o capture-detector.exe -O2

# macOS
clang source/capture-detector.c -o capture-detector -framework CoreGraphics -framework CoreFoundation

# Linux
gcc source/capture-detector.c -o capture-detector -lX11
```

## License

MIT · coded by AsoiX

## 🤝 Thanks to the contributors

For me (**[AsoiX](https://github.com/AsoiX-exe)**), CaptureDetector is more than a small
utility — it's a tool built to answer one very concrete question: what is quietly keeping
itself out of your screenshots and recordings. That said, I won't pretend the shape it's in
today is a one-person job. A lot of what makes it solid — cleaner builds, cross-platform
support, the awkward edge cases I'd never have caught on my own — grew out of people who
actually sat down with it, asked the uncomfortable questions and pointed at the things I had
overlooked. Every pull request, fix, suggestion and bug report along the way moved the project
a step forward, and I don't take that for granted. If you'd like to be part of it, have a look
at the [contributing guide](https://github.com/AsoiX-exe/CaptureDetector/blob/master/CONTRIBUTING.md).
