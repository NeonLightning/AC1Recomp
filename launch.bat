@echo off
setlocal
set "ROOT=%~dp0"
if not defined AC1RECOMP_BUILD_DIR set "AC1RECOMP_BUILD_DIR=%ROOT%build-release"
if not exist "%AC1RECOMP_BUILD_DIR%\ArmoredCoreLauncher.exe" (
    echo Missing executable: %AC1RECOMP_BUILD_DIR%\ArmoredCoreLauncher.exe 1>&2
    exit /b 1
)
"%AC1RECOMP_BUILD_DIR%\ArmoredCoreLauncher.exe" %*
exit /b %ERRORLEVEL%
