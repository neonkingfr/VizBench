default:
	nmake PYTHON=c:\python27 install
	c:\python27\python setup.py py2exe
	copy dist\jsonrpc.exe ..\..\bin

justinstall:
	nmake PYTHON=c:\python27 install

FINALDIR = $(PYTHON)\lib\site-packages\nosuch

install : \
	$(FINALDIR) \
	$(FINALDIR)\jsonrpc.py

$(FINALDIR):
	mkdir $(FINALDIR)

$(FINALDIR)\__init__.py: __init__.py
	copy __init__.py $(FINALDIR)\__init__.py

$(FINALDIR)\jsonrpc.py: jsonrpc.py
	copy jsonrpc.py $(FINALDIR)\jsonrpc.py

clean :
	rm -f *~ *.bak *.pyc
