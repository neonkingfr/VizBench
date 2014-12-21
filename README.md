manifold
========

Space Manifold - a host and framework for FFGL plugins that involve
OSC/TUIO and HTTP listeners and MIDI I/O.

It's a work in progress, I'm putting this out in barely-usable form
so people on the FreeFrame developers mailing list can take a look.

Here are the steps to exercise the things that work:

   - Clone the Github repository http://github.com/nosuchtim/manifold

   - Install Visual Studio 2013 Community edition

   - Set environment variable VIZBENCH to
        c:/users/YOURLOGIN/documents/github/manifold
     replacing YOURLOGIN with your Windows login name.

   - Add $VIZBENCH/bin to your PATH

At this point you should have enough to able to execute these commands:

    cd $VIZBENCH/bin
    FFFF.exe
    p fourcircles.osc

and see some graphics appear in the FFFF window.  FFFF.exe is an FFGL
host that can be configured to load a pipeline of FFGL plugins.
FFFF.exe is configured with the config/ffff.json file,
and the "initialconfig" value in ffff.json points to a file in
the config/ffff directory.  An "initialconfig" of "vizdraw" will
load the config/ffff/vizdraw.json file - a single FFGL plugin that listens
for TUIO/OSC messages to draw graphics.  The "p fourcircles.osc" command
(see above) replays some TUIO/OSC messages to draw things.
See src/plugins/VizDraw/VizDraw.cpp for the source.

You can set the "initialconfig" to "vizbox2d".  VizBox2d is a plugin that
uses the Box2D physics engine, and you can interact with it using keyboard
commands.  After invoking FFFF.exe, press 't' or 'r' a few times
to add some circles to the screen, and then press 'f' or 'g' to push
them around.  See src/plugins/VizBox2d/VizBox2d.cpp for the source.

You can set the "initialconfig" to "vizmidi".  VizMidi is a plugin that
listens for MIDI input and draws graphics.  It is also capable of playing
MIDI files.  See config/spaceserver.json to set the MIDI input/output devices.
See src/plugins/VizMidi/VizMidi.cpp for the source.


To recompile everything from scratch:

   - Download http://blueparticles.com/particles/other.zip and
     unzip it into the manifold directory.  This file contains lots
     of things that are needed to recompile everything successfully

   - Start Visual Studio 2013 on $VIZBENCH/vs2013/SpaceManifold.sln
     and build the entire solution.
