@echo off
echo Installing FTDI Driver...

ver | findstr /c:"Version 5." > nul
if %ERRORLEVEL%==0 goto :install_old

if DEFINED PROCESSOR_ARCHITEW6432 goto :install_new_wow

:install_new
if DEFINED PROCESSOR_ARCHITECTURE goto :install_new_64
"%~dp0\dpinst_x86.exe" /se /sw
exit

:install_new_64
if %PROCESSOR_ARCHITECTURE%==x86 goto :install_new_32
"%~dp0\dpinst_amd64.exe" /se /sw
exit

:install_new_32
"%~dp0\dpinst_x86.exe" /se /sw
exit

:install_new_wow
"%~dp0\dpinst_amd64.exe" /se /sw
exit

:install_old
"%~dp0\dpinst_x86.exe" /se /sw
exit
