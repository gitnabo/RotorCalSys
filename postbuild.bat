@echo on
set QTBINDIR=%1
set OUTDIR=%2%3
set EXE=RotorSysCal.exe
echo postbuild.bat: Windeploy
echo OUTDIR=%OUTDIR%
echo QTBINDIR=%QTBINDIR%
%QTBINDIR%\windeployqt.exe %OUTDIR%%EXE%

time /t > %OUTDIR%postbuild.trg