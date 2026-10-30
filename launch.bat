@echo off
setlocal
set "ROOT=%~dp0"
if not defined AC1RECOMP_BUILD_DIR set "AC1RECOMP_BUILD_DIR=%ROOT%build-release"
echo Armored Core Recomp
echo.
echo 1^) Armored Core
echo 2^) Project Phantasma
echo Q^) Quit
choice /C 12Q /N /M "> "
if errorlevel 3 exit /b 0
if errorlevel 2 (
    if not exist "%AC1RECOMP_BUILD_DIR%\ArmoredCoreProjectPhantasma.exe" (
        echo Missing executable: %AC1RECOMP_BUILD_DIR%\ArmoredCoreProjectPhantasma.exe 1>&2
        exit /b 1
    )
    "%AC1RECOMP_BUILD_DIR%\ArmoredCoreProjectPhantasma.exe" %*
    exit /b %ERRORLEVEL%
)
if not exist "%AC1RECOMP_BUILD_DIR%\AC1.exe" (
    echo Missing executable: %AC1RECOMP_BUILD_DIR%\AC1.exe 1>&2
    exit /b 1
)
"%AC1RECOMP_BUILD_DIR%\AC1.exe" %*
exit /b %ERRORLEVEL%
