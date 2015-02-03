VizBench
========

A framework for creating FreeFrame 1.5 (FFGL) plugins that involve
OSC/TUIO and HTTP listeners and MIDI I/O.  Plugins listen for
HTTP on port 80 (by default) in order to deliver HTML GUIs and respond
to a variety of APIs.  Plugins listen for OSC/TUIO on port 3333 in order
to respond to cursor information.

The FFGL plugins here have been tested in Resolume (4.1.11) and Magic (1.53).

For people that don't want to compile anything, or want to try it out
before compiling anything, an installer is available, see http://vizicist.com

Also included is a FreeFrame host (FFFF) for testing and simple uses.
FFFF is capable of loading a pipeline of FFGL plugins.

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
        c:\users\YOURLOGIN\documents\github\VizBench
     replacing YOURLOGIN with your Windows login name.  You can do this in
     the Control Panel, under System->Advanced System Settings->Advanced
     ->Environment Variables.  Note that BACKSLASHES are used in this path.
     Some scripts will break ifyou use forward slashes.

   - Install Visual Studio 2013 Community edition (it's free).
     You can also use Visual Studio 2010 if you prefer.

   - Install Python 2.7 from https://www.python.org/download/releases/2.7/
     and leave it in the default location - C:\Python27.

   - Add %VIZBENCH%/bin to your PATH

   - Start Visual Studio 2013 or Visual Studio 2010 with this solution:

         build/vs2010_or_vs2013/VizBench.sln

   - Make sure Visual Studio is set to compile the Debug Win32 version.

   - Compile the entire solution.

============================================================================
At this point you should have enough to execute this first example:

    cd %VIZBENCH%/bin
    FFFF VizExample1

You should see a window with a single red outlined square.  If that works,
you can move on to the second example:

    FFFF VizExample2
    p fourcircles.osc

The second line there uses the p.bat script to replay a series of TUIO/OSC
messages that get sent to port 3333 (a standard port for TUIO messages)
to convey mouse cursor movement.  When these messages are received,
they will result in some graphics (little sprites) in the FFFF window.

If you have an iPhone or iPad with the TuioPad app, you can use TuioPad to
draw things in VizExample2.  Just start TuioPad, set the host IP address
to the IP address of your Windows machine, press the Start button,
and start drawing with your fingers.  Note that it's multitouch -
you can use more than one finger at a time.

============================================================================
Next example: vizlife

    FFFF vizlife
    api VizLife.randomize

This example loads the VizLife plugin, which plays Conway's Game of Life.
Invoking the "VizLife.randomize" api, as shown, will randomly fill cells.
You'll notice that when cells are "born", they cause sprites to be created.
Invoking the "VizLife.clear" api will clear the grid.

============================================================================
Next example: vizshader

    FFFF vizshader
    p viztest.osc

The second line there uses the p.bat script to replay a series of
OSC messages that get sent to port 3333 (a standard for TUIO messages)
to convey mouse cursor movement.  The VizShader plugin uses code
from Lynn Jarvis' Shader Maker (see https://github.com/leadedge/ShaderMaker)
that creates an FFGL plugin directly from shader code you can find at the
GLSL Sandbox and ShaderToy sites.

A current wrinkle of this example is that FFGL Host control of the
"X Mouse" and "Y Mouse" parameters is disabled as soon as the first TUIO/OSC
message is received.  This means that if you use the plugin inside Resolume
or Magic, the "X Mouse" and "Y Mouse" parameter control in their GUIs
will no longer work after you start using TUIO/OSC.  If you don't use TUIO/OSC,
the GUI-controlled parameters work normally.

============================================================================
Next example: vizmidi

    FFFF vizmidi
    mf.bat prelude.mid

This example will "play" the midifiles/prelude.mid, and draw
sprites corresponding to the notes.  It will also send the MIDI notes to
the MIDI output device specified in the config/vizserver.json file.
Most of the time, you'll be using a virtual MIDI mechanism like
loopMIDI or LoopBe30 to route MIDI between applications.
Be warned - sometime these MIDI routing mechanisms can shut down
unexpectedly or (more likely) disable ports when MIDI feedback is detected.

============================================================================
Next example: vizbox2d

    FFFF vizbox2d
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

POTENTIAL ISSUES:

- You can only run one FFFF program at a time, since it listens on several
  ports (for HTTP and OSC) and only one program can listen on them at a time.

- If you run one of these plugins inside a FreeFrame host like Resolume,
  the plugin must be active before you can talk to its web interface.

============================================================================

Questions: me@timthompson.com
