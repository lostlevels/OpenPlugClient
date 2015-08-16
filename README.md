# CLI client for [OpenPlugServer](https://github.com/DerrickGold/OpenPlugServer)

### Screeny
![picture](https://raw.githubusercontent.com/lostlevels/OpenPlugClient/master/screen.png)

### Depedencies
 * libavcodec
 * PortAudio
 * RapidJSON
 * youtube-dl
 * curses
 * happyhttp (included)

### Building
* run premake from main folder

### Usage
 * Retrieve playlist songs (optional): ```playlist playlist_name```
 * play playlist : ```playlist playlist_name play```
 * add playlist foo : ```playlist foo add```
 * add song on [youtube](https://www.youtube.com/watch?v=5ShzyggtsCs) to playlist foo : ```playlist foo add https://www.youtube.com/watch?v=5ShzyggtsCs```

