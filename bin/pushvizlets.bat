copy c:\users\tjt\documents\github\vizlets\installer\vizlets\vizlets\vizlets-setup.exe p:\tmp

p:
cd \tmp
set vizlets=vizlets-setup.exe

set PATH=c:\local\rsync;%PATH%
set CYGWIN=tty binmode
set TERM=ansi
rem set RSYNC_RSH=c:\local\rsync\ssh.exe
set RSYNC_RSH=/cygdrive/c/local/rsync/ssh.exe
set USERNAME=tjt
set HOME=c:\local\rsync
echo Drive C >>c:\local\rsync\rsync.log
chmod -R +rx "%vizlets%"
c:\local\rsync\rsync -av "%vizlets%" tjt@nosuch.com:/htdocs/tmp/vizlets