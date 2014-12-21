import time
import mmtt.util
import mmtt.builtin
import nosuch.midiutil
from mmtt.builtin import debug
from nosuch.midiutil import Midi

# a = mmtt.util.getprocessor("Default","../")
a = mmtt.util.getprocessor("Default")
debug("before a.draw 1")
a.draw()
debug("before a.draw 2")
a.draw()
time.sleep(4.0)
Midi.shutdown()
time.sleep(1.0)
