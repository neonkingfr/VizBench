Directories here:

pyffle/processor - contains FFGL plugins written in Python.

pyffle/builtin - a module that is normally not used, but is useful for testing
         code outside of the actual Pyffle plugin, which normally provides
         the pyffle module automatically, implemented in C code of the plugin.

pyffle/util - utility functions, used internally by Pyffle

nosuch - a module of utilities not absolutely needed by Pyffle.
         Some of them may assume that other non-standard
         Python modules are available (e.g. simpleOSC).
