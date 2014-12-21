from nosuch.midiutil import *
from nosuch.midifile import *
from nosuch.midipypm import *
from nosuch.midivsthost import *
from traceback import format_exc
from time import sleep
import sys

if __name__ == '__main__':

	if len(sys.argv) < 2 or len(sys.argv) > 3:
		print "Usage: midiplay {filename} [ {outputdevice} ] "
		sys.exit(1)
	
	fname = sys.argv[1]
	if len(sys.argv) > 2:
		outdevice = sys.argv[2]
	else:
		outdevice = None


	Midi.startup()

	if outdevice == "VstHost":
		m = MidiVstHostHardware()
		outputname = None   # so it will open the default output
	else:
		m = MidiPypmHardware()
		outputname = outdevice

	# If you want to get debug output (e.g. to see when
	# things are written to MIDI output), use this:
	# Midi.debug = True

	# Read the midifile
	p = Phrase.fromMidiFile(fname)

	# Open MIDI output named
	try:
		# if outdevice is None, it opens the default output
		o = m.get_output(outputname)
		o.open()
	except:
		print "Error opening output: %s, exception: %s" % (outputname,format_exc())
		Midi.shutdown()
		sys.exit(1)

	# Schedule the notes of the midifile on the output
	for n in p:
		o.schedule(n)

	# sleep/wait till all the notes are played
	try:
		while Midi.num_scheduled() > 0:
			sleep(1.0)
	except KeyboardInterrupt:
		print "Got KeyboardInterrupt!"
	except:
		print "Unexpected exception: %s" % format_exc()

	Midi.shutdown()
