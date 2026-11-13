<h1 align="center">
    Fletch
</h1>
<p align="center">
    is a simple system information tool written in C and forked from efetch
</p>
<br>

## OS Support
Fletch has good OS detection on almost all UNIX systems however package
manager detection is less complete
Fletch currently supports the following package managers:
- emerge
- pacman
- xbps
- pkg
- dpkg (apt)

If the package manager you use is not listed feel free to send a pull request
to add support or open an issue

## Configuration
There is a ``config.h`` file included with the source code where you can
toggle certain fields and edit the current color scheme.

## Installation
  - You are able to use `make` in order to update and/or install ``efetch``
