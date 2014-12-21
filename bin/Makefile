default:
	nmake PYTHON=c:\python26 install
	nmake PYTHON=c:\python27 install

justinstall:
	nmake PYTHON=c:\python26 install
	nmake PYTHON=c:\python27 install

FINALDIR = $(PYTHON)\lib\site-packages\nosuch

install : \
	$(FINALDIR)\__init__.py \
	$(FINALDIR)\vsthost.py

$(FINALDIR)\__init__.py: __init__.py
	copy __init__.py $(FINALDIR)\__init__.py

$(FINALDIR)\vsthost.py: vsthost.py
	copy vsthost.py $(FINALDIR)\vsthost.py

clean :
	rm -f *~ *.bak *.pyc

dist :
	rm -f *~ *.bak *.pyc
	cd ..
	del /q python_nosuch_vsthost.zip
	zipdir nosuch_vsthost
	mv nosuch_vsthost.zip python_nosuch_vsthost.zip
	tosoft python_nosuch_vsthost.zip
	cd nosuch_vsthost
