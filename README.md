# Nanosaur source port

This is _Nanosaur_ for modern operating systems (macOS, Windows, Linux). This version, at https://github.com/jorio/nanosaur, is approved by Pangea Software.

Get builds for macOS and Windows here: https://github.com/jorio/nanosaur/releases

![Screenshot](docs/screenshot.png)

## How to build

To build the game yourself, please read [BUILD.md](BUILD.md).

## About this port

### Context

Nanosaur is a 1998 Macintosh game by Pangea Software. In it, you’re a cybernetic dinosaur from the future who’s sent back in time 20 minutes before a giant asteroid hits the Earth. And you get to shoot at T-Rexes with nukes.

Nanosaur was bundled with the original iMac and ran on Mac OS 8. It’s also notable for being a prominent showcase of QuickDraw 3D’s capabilities, which was Apple’s high-level 3D graphics API during the 90s.

In 1999, Pangea released [Nanosaur’s source code](http://www.pangeasoft.net/nano/nanosource.html) to the public. This port is based on that release.

### Port philosophy

I took a conservative approach to port the game, in a similar spirit to “Chocolate Doom”. I aimed to accurately maintain the behavior of the original game. The port has some minor quality-of-life improvements such as support for arbitrary resolutions. Modifications to the gameplay or presentation are out of the scope of this project.

To make it easier to port the game, I wrote an implementation of parts of the Macintosh Toolbox API, which I called “[Pomme](https://github.com/jorio/Pomme)”. You can think of Pomme as a cross-platform reimagining of [Carbon](https://en.wikipedia.org/wiki/Carbon_(API)), albeit at a much smaller scope.

Rendering is handled via [Quesa](https://github.com/jorio/Quesa), an independent implementation of QuickDraw 3D.

### Credits

Nanosaur™ © 1998 Pangea Software, Inc. Ported by Iliyas Jorio with permission by Pangea Software.

### See also: Bugdom

If you like Pangea games, check out [my remastered version of the original Bugdom](https://github.com/jorio/Bugdom).