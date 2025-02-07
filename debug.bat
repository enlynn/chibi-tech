@echo off
setlocal EnableDelayedExpansion

:: Project directories
SET HOST_DIR=%~dp0
SET HOST_DIR=%HOST_DIR:~0,-1%

SET BUILD_DIR_DBG=%HOST_DIR%\bin\debug
SET BUILD_DIR_OPT=%HOST_DIR%\bin\opt
SET BUILD_DIR_OPTDBG=%HOST_DIR%\bin\optwithdebug
SET BUILD_DIR_MINSZOPT=%HOST_DIR%\bin\minsizeopt

pushd %BUILD_DIR_DBG%
	ninja
popd