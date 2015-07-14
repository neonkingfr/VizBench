default install:
	nmake PYTHON=c:\python27 doinstall
	c:\python27\python setup.py py2exe
	..\..\bin\postpy2exe.bat

justinstall:
	nmake PYTHON=c:\python27 doinstall

justsetup:
	c:\python27\python setup.py py2exe
	copy dist\oscutil.exe ..\..\bin

justcombine:
	..\..\bin\combinelibrary.bat

FINALDIR = $(PYTHON)\lib\site-packages\nosuch

doinstall : \
	$(FINALDIR) \
	$(FINALDIR)\__init__.py \
	$(FINALDIR)\oscutil.py

$(FINALDIR):
	mkdir $(FINALDIR)

$(FINALDIR)\__init__.py: __init__.py
	copy __init__.py $(FINALDIR)\__init__.py

$(FINALDIR)\oscutil.py: oscutil.py
	copy oscutil.py $(FINALDIR)\oscutil.py

clean :
	rm -fr *~ *.bak *.pyc dist build

# dist :
# 	rm -f *~ *.bak *.pyc
# 	cd ..
# 	del /q python_nosuch_oscutil.zip
# 	zipdir nosuch_oscutil
# 	mv nosuch_oscutil.zip python_nosuch_oscutil.zip
# 	tosoft python_nosuch_oscutil.zip
# 	cd nosuch_oscutil
