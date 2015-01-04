VizBench
========

A framework for creating FreeFrame 1.5 (FFGL) plugins that involve
OSC/TUIO and HTTP listeners and MIDI I/O.

Also included is a FreeFrame host (FFFF) for testing and simple uses.

The FFGL plugins here have been tested in Resolume (4.1.11).

This is a work in progress, but if anything in the instructions below
doesn't work as advertized (on Window 8 or Windows 7), please send
email to me@timthompson.com and let me know.

    ...Tim...

============================================================================
Here are the steps to obtain and build it:

   - Clone the Github repository http://github.com/nosuchtim/VizBench.
     It should end up in c:/users/YOURLOGIN/documents/github/VizBench,
     where YOURLOGIN is your windows login name.

   - Set environment variable VIZBENCH to
        c:/users/YOURLOGIN/documents/github/VizBench
     replacing YOURLOGIN with your Windows login name.  You can do this in
     the Control Panel, under System->Advanced System Settings->Advanced
     ->Environment Variables.

   - Install Visual Studio 2013 Community edition (it's free)

   - Install Python 2.7 from https://www.python.org/download/releases/2.7/
     and leave it in the default location - C:\Python27.

   - Add %VIZBENCH%/bin to your PATH

   - Start Visual Studio, loading the build/vs2013/VizBench.sln solution.

   - Make sure Visual Studio is set to compile the Debug Win32 version.

   - Compile the entire solution.

============================================================================
At this point you should have enough to execute this first example:

    cd %VIZBENCH%/bin
    FFFF.exe vizdraw
    p.bat fourcircles.osc

and see some graphics appear in the FFFF window.  FFFF.exe is an FFGL
host that can be configured to load a pipeline of FFGL plugins.
The command-line argument to FFFF.exe is the initial configuration to load,
which should be the name of a file in the config/ffff directory.
In the example above, the file config/ffff/vizdraw.json is used, which
loads the single FFGL plugin named VizDraw, which listens for TUIO/OSC
messages to draw graphics.  The "p.bat fourcircles.osc" command 
replays some TUIO/OSC messages stored in the fourcircles.osc file.

============================================================================
Second example: vizmidi

    FFFF.exe vizmidi
    mf.bat prelude.mid

This example will "play" the midifiles/prelude.mid, and draw
sprites corresponding to the notes.  It will also send the MIDI notes to
the MIDI output device specified in the config/vizserver.json file.

============================================================================
Third example: vizlife

    FFFF.exe vizlife
    api VizLife.randomize

This example loads the VizLife plugin, which plays Conway's Game of Life.
Invoking the "VizLife.randomize" api, as shown, will randomly fill cells.
You'll notice that when cells are "born", they cause sprites to be created.
Invoking the "VizLife.clear" api will clear the grid.

============================================================================
Fourth example: vizbox2d

    FFFF.exe vizbox2d
    api VizBox2d.randomize
    api VizBox2d.push

The VizBox2d.randomize api will randomly place some objects, and
VizBox2d.push will push them around under the control of the Box2D engine.
============================================================================

To create your own Vizlet-based FFGL plugin (we'll call it MyViz), execute:

    genvizlet MyViz

which will generate all of the source and build files needed.  You should
add it to the Visual Studio solution by right-clicking on the "plugins"
folder (in Visual Studio), using "Add->Existing Project...", and selecting
the build/vs2013/MyViz/MyViz.vcxproj file.  The source code for the plugin
will be in src/plugins/MyViz.  Compiling the solution should then result
in creating this FFGL plugin:

    ffglplugins\MyViz.dll

which you can use like any of the other Vizlet-based plugins.
============================================================================

Questions: email me@timthompson.com
