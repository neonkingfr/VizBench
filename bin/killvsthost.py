from subprocess import call, Popen
from time import sleep

def killtask(nm):
	call(["c:/windows/system32/taskkill","/f","/im",nm])

killtask("vsthost.exe")

