@echo off
setlocal EnableDelayedExpansion

:: Project directories
SET HOST_DIR=%~dp0
SET HOST_DIR=%HOST_DIR:~0,-1%

SET BUILD_DIR_OPT=%HOST_DIR%\bin\opt

pushd %BUILD_DIR_OPT%
	ninja
popd