default install:
	nmake PYTHON=c:\python27 doinstall
	c:\python27\python setup.py py2exe
	..\..\bin\postpy2exe.bat

justinstall:
	nmake PYTHON=c:\python27 doinstall

FINALDIR = $(PYTHON)\lib\site-packages\nosuch

doinstall : \
	$(FINALDIR) \
	$(FINALDIR)\jsonrpc.py

$(FINALDIR):
	mkdir $(FINALDIR)

$(FINALDIR)\__init__.py: __init__.py
	copy __init__.py $(FINALDIR)\__init__.py

$(FINALDIR)\jsonrpc.py: jsonrpc.py
	copy jsonrpc.py $(FINALDIR)\jsonrpc.py

clean :
	rm -fr *~ *.bak *.pyc dist build
