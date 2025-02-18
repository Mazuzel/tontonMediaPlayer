# Tonton Media Player
Audio - midi - video control for live music

## Features
### Groove machines control
Define and play songs for your Elektron Digitakt, Elektron Model:Cycles, or any groove machine.
Tonton Media Player handles the master clock, and sends program changes when it needs to.

### Backing tracks
Backing tracks are played in sync with your groove machines.

### Video player
Video player with video mapping capability, playing your clips in sync with your song.

## Configuration
Almost everything has to be configured manually through xml files (setlist, hardware config, songs). This provides a very low risk of mishandling when using the software.

Each song must have a corresponding folder in the ./songs directory

For each song, that folder must at least contain a 'structure.xml' file defining the song parts, bpms, midi patches, etc...

Additionally, backing track files can be added in an 'audio' subdirectory.
Only '.wav', '.flac' and '.mp3' files are allowed.

Furthermore, video clip can be added in a 'clip' subdirectory.
The video clip must be named 'clip.mp4'.

## Keyboard commands
s: select 'Setlist' panel
b: select 'Backing tracks' panel
m: select 'Midi outputs' panel

v: show/hide video mapping control screen

When 'Setlist' panel is selected:
right: play song
left: stop and reload song
top/bottom: navigate setlist
enter: load selected song

When 'Backing tracks' panel is selected:
top/bottom: navigate backing tracks files
left/right: adjust volume

When 'Midi outputs' panel is selected:
top/bottom: navigate midi outputs
left/right: manually change patch (only for patch_format outputs)

Shift+Q: Quit app
