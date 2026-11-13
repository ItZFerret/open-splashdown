Splashdown PC port - Windows test build
========================================

This is an early, untested Windows build. It has only been run on macOS so
far, so expect rough edges - a crash at start-up is useful news.

You need
--------
- 64-bit Windows 10 or 11, a CPU with AVX2 (anything from about 2014 on).
- Your own disc image of Splashdown for PlayStation 2, the US release
  (NTSC-U, SLUS-20223), as an .iso file. Other regions' discs do not work.
  The image is not part of this package.

To play
-------
1. Unzip this folder anywhere (not inside Program Files).
2. Put your Splashdown .iso into this folder, next to splashdown.exe.
3. Double-click "Play Splashdown.bat".
   (Or drop the .iso onto "Play Splashdown.bat" from wherever it is.)

A window opens after a few seconds of loading. The publisher logos and the
intro movie play first; Enter skips.

Controls
--------
Any Xbox or PlayStation style gamepad works (A/B/X/Y = Cross/Circle/
Square/Triangle, Menu = Start, View = Select).

Keyboard: arrow keys = D-pad, W A S D = left stick, I J K L = right stick,
X or Space = Cross (accelerate / confirm), C or Esc = Circle (back),
Z = Square, V = Triangle, Enter = Start, Tab = Select, Q / E = L1 / R1,
1 / 3 = L2 / R2.

Press F1 for the display settings: internal resolution (2x, 3x), widescreen
(16:9, 21:9 ...), fullscreen,
window size, scaling, VSync, a frame counter, and Skip intro (start at the
menu without the logos and intro movie).

GPU renderer (experimental, new in this build)
----------------------------------------------
By default the picture is drawn on the CPU. In the F1 settings, set
Renderer to GPU and restart the game to draw it on the graphics card
instead, which keeps full speed at 2x and 3x resolution. It needs a
graphics driver with Vulkan (current Nvidia, AMD and Intel drivers have
it). This has never been run on Windows: if the picture is wrong or the
game won't start, set Renderer back to Software (or delete
%APPDATA%\splashdown\settings.ini) and send splashdown-log.txt - the
lines starting "GPU:" say what happened.

Saves go to %APPDATA%\splashdown\memcard0.

If something goes wrong
-----------------------
"Play Splashdown.bat" writes everything the game prints to
splashdown-log.txt in this folder and shows its last lines if the game
stops with an error. Please send that file back, with:
- what you saw (nothing at all, an error box, a black window, a freeze ...),
- how far it got (logos, menu, loading a race, in a race),
- your CPU and Windows version.

Files
-----
splashdown.exe                 the game
SDL3.dll                       window, sound and gamepads (zlib licence)
avcodec / avutil / swresample  FFmpeg, for the movies (LGPL)
