import sys
import os
import re
import hashlib

nm = sys.argv[1]
id = "V%03d" % (int(hashlib.sha1(nm).hexdigest(), 16) % (10**3))
print "4-character FFGL ID for "+nm+" is "+id
