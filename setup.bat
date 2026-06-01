@echo off
REM SPDX-License-Identifier: LGPL-3.0-or-later
REM Copyright (c) 2026 Cryptnox SA
setlocal EnableDelayedExpansion

echo ============================================
echo  CryptnoxWallet Arduino - Library Setup
echo ============================================
echo.

set "ARDUINO_LIBS=%USERPROFILE%\Documents\Arduino\libraries"
set "SCRIPT_DIR=%~dp0"
set "RESET_MODE=0"

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--reset" (
    set "RESET_MODE=1"
    shift
    goto :parse_args
)
set "ARDUINO_LIBS=%~1"
shift
goto :parse_args
:args_done

echo Arduino libraries directory: %ARDUINO_LIBS%
if "%RESET_MODE%"=="1" echo Mode: --reset (wipe entire libraries directory before install)
echo.

if not exist "%ARDUINO_LIBS%" (
    echo Creating %ARDUINO_LIBS%...
    mkdir "%ARDUINO_LIBS%"
)

REM Backup existing libraries (sibling of ARDUINO_LIBS so it follows any custom path)
for /f %%I in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss" 2^>nul') do set "DT=%%I"
set "BACKUP_DIR=%ARDUINO_LIBS%_backup_%DT%"
echo Creating backup at:
echo   %BACKUP_DIR%
xcopy /E /I /Q /Y "%ARDUINO_LIBS%" "%BACKUP_DIR%" >nul 2>&1
echo Backup done.
echo.

REM Optionally wipe the libraries directory (--reset)
if "%RESET_MODE%"=="1" (
    echo [RESET] Wiping %ARDUINO_LIBS%...
    rmdir /S /Q "%ARDUINO_LIBS%"
    mkdir "%ARDUINO_LIBS%"
    echo Done.
    echo.
)

REM git is the only install method
where git >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: git is not available.
    echo Please install git from: https://git-scm.com/
    exit /b 1
)

echo Initialising git submodules...
git -C "%SCRIPT_DIR:~0,-1%" submodule update --init --recursive
echo.

REM Install SDK itself (selective: only the files Arduino needs, not patches/docs/scripts)
set "SDK_DEST=%ARDUINO_LIBS%\CryptnoxWallet"
echo Installing CryptnoxWallet...
if exist "%SDK_DEST%" rmdir /S /Q "%SDK_DEST%"
mkdir "%SDK_DEST%"
xcopy /I /Q /Y "%SCRIPT_DIR%library.properties" "%SDK_DEST%\" >nul 2>&1
xcopy /I /Q /Y "%SCRIPT_DIR%keywords.txt"        "%SDK_DEST%\" >nul 2>&1
xcopy /I /Q /Y "%SCRIPT_DIR%LICENSE"             "%SDK_DEST%\" >nul 2>&1
for %%F in ("%SCRIPT_DIR%README*") do xcopy /I /Q /Y "%%F" "%SDK_DEST%\" >nul 2>&1
xcopy /E /I /Q /Y "%SCRIPT_DIR%src"      "%SDK_DEST%\src\"      >nul
xcopy /E /I /Q /Y "%SCRIPT_DIR%examples" "%SDK_DEST%\examples\" >nul
REM Drop subdirectories from the cpp submodule that the Arduino IDE would
REM otherwise recursively compile (and link, causing multiple-definition
REM errors): fuzz harnesses, doxygen docs, hidden CI dirs.
if exist "%SDK_DEST%\src\cryptnox-sdk-cpp\fuzz"   rmdir /S /Q "%SDK_DEST%\src\cryptnox-sdk-cpp\fuzz"
if exist "%SDK_DEST%\src\cryptnox-sdk-cpp\docs"   rmdir /S /Q "%SDK_DEST%\src\cryptnox-sdk-cpp\docs"
if exist "%SDK_DEST%\src\cryptnox-sdk-cpp\.github" rmdir /S /Q "%SDK_DEST%\src\cryptnox-sdk-cpp\.github"
echo   OK
echo.

REM ---------------------------------------------------------------------------
REM Required library versions (checked against installed library.properties)
REM ---------------------------------------------------------------------------
set "REQ_AESLib=2.3.6"
set "REQ_Adafruit_BusIO=1.17.4"
set "REQ_Adafruit_PN532=1.3.4"
set "REQ_ArduinoHttpClient=0.6.1"
set "REQ_Crypto=0.4.0"
set "REQ_micro_ecc=1.0.0"
set "REQ_trng=1.0.2"

echo Installing dependency libraries...
echo.

call :install_git "AESLib"            "%REQ_AESLib%"            "AESLib"            "https://github.com/suculent/thinx-aes-lib"
call :install_git "Adafruit BusIO"    "%REQ_Adafruit_BusIO%"    "Adafruit_BusIO"    "https://github.com/adafruit/Adafruit_BusIO"
call :install_git "Adafruit PN532"    "%REQ_Adafruit_PN532%"    "Adafruit_PN532"    "https://github.com/adafruit/Adafruit-PN532"
call :install_git "ArduinoHttpClient" "%REQ_ArduinoHttpClient%" "ArduinoHttpClient" "https://github.com/arduino-libraries/ArduinoHttpClient"
call :install_git "micro-ecc"         "%REQ_micro_ecc%"         "micro-ecc"         "https://github.com/kmackay/micro-ecc"
call :install_git "trng"              "%REQ_trng%"              "trng"              "https://github.com/embarquech/trng"
call :install_git_crypto "Crypto"     "%REQ_Crypto%"

REM ---------------------------------------------------------------------------
REM Apply library patches (patches\ directory)
REM ---------------------------------------------------------------------------
REM Sentinel files: renesas.patch targets AES_config.h (and xbase64.cpp);
REM no_iostream.patch targets AESLib.h (and AESLib.cpp). The two patches
REM inject overlapping ARDUINO_ARCH_RENESAS_UNO substrings, so a recursive
REM 'findstr /s' check would conflate them - point each sentinel scan at
REM the file the patch actually rewrites.
call :apply_patch "AESLib"         "%ARDUINO_LIBS%\AESLib"         "patches\AESLib_renesas.patch"         "ARDUINO_ARCH_RENESAS"     "src\AES_config.h"
call :apply_patch "AESLib"         "%ARDUINO_LIBS%\AESLib"         "patches\AESLib_no_iostream.patch"     "ARDUINO_ARCH_RENESAS"     "src\AESLib.h"
call :apply_patch "Adafruit PN532" "%ARDUINO_LIBS%\Adafruit_PN532" "patches\Adafruit_PN532_timeout.patch"        "waitready(10000)"
REM Extended-frame patch bumps PN532_PACKBUFFSIZ 64->440 and adds
REM inDataExchange16 so cert pages > ~255 bytes deliver intact.
call :apply_patch "Adafruit PN532" "%ARDUINO_LIBS%\Adafruit_PN532" "patches\Adafruit_PN532_extended_frame.patch" "inDataExchange16"

REM ---------------------------------------------------------------------------
REM Memory optimization (platform.local.txt + Arduino core dtostrf patch)
REM ---------------------------------------------------------------------------
echo.
echo Running memory optimization...
call "%SCRIPT_DIR%scripts\enable_memory_optimization.bat"
if errorlevel 1 (
    echo   WARNING: memory optimization failed. Sketches will still compile
    echo   but will use ~145 KB more flash than necessary.
)

echo.
echo ============================================
echo  Setup complete! Restart Arduino IDE.
echo ============================================
endlocal
exit /b 0

REM ---------------------------------------------------------------------------
REM :apply_patch <label> <lib_dir> <patch_file> <sentinel> [sentinel_file]
REM   Applies a unified-diff patch to <lib_dir> using git apply.
REM   Skips silently if <sentinel> string is already present.
REM   If <sentinel_file> is given, only that file (relative to <lib_dir>) is
REM   scanned - lets two patches that touch the same library use overlapping
REM   sentinel substrings without one masking the other. If omitted, the
REM   whole lib tree is scanned recursively (legacy behaviour).
REM ---------------------------------------------------------------------------
:apply_patch
set "_LABEL=%~1"
set "_LIB=%~2"
set "_PATCH=%SCRIPT_DIR%%~3"
set "_SENTINEL=%~4"
set "_SENTINEL_FILE=%~5"
if not exist "%_LIB%" (
    echo [SKIP] %_LABEL% not found, skipping patch.
    echo.
    exit /b 0
)
if not exist "%_PATCH%" (
    echo [SKIP] Patch file %~3 not found.
    echo.
    exit /b 0
)
if defined _SENTINEL_FILE (
    findstr /r /c:"%_SENTINEL%" "%_LIB%\%_SENTINEL_FILE%" >nul 2>&1
) else (
    findstr /r /s /c:"%_SENTINEL%" "%_LIB%\*" >nul 2>&1
)
if %ERRORLEVEL% == 0 (
    echo [OK] %_LABEL% patch already applied.
    echo.
    exit /b 0
)
echo Patching %_LABEL%...
set "_LIB_FWD=%_LIB:\=/%"
git apply --unsafe-paths --directory="%_LIB_FWD%" "%_PATCH%"
if %ERRORLEVEL% == 0 (
    echo   OK
) else (
    echo   WARNING: patch failed for %_LABEL%. Apply manually: %~3
)
echo.
exit /b 0

REM ---------------------------------------------------------------------------
REM :get_installed_version <dir> -> sets INSTALLED_VER (empty if not found)
REM ---------------------------------------------------------------------------
:get_installed_version
set "INSTALLED_VER="
set "_PROPS=%ARDUINO_LIBS%\%~1\library.properties"
if not exist "%_PROPS%" exit /b 0
for /f "tokens=1,* delims==" %%A in ('type "%_PROPS%"') do (
    if /i "%%A"=="version" set "INSTALLED_VER=%%B"
)
exit /b 0

REM ---------------------------------------------------------------------------
REM :version_gte <verA> <verB> -> exit 0 if verA >= verB, else exit 1
REM ---------------------------------------------------------------------------
:version_gte
set "_VA=%~1"
set "_VB=%~2"
for /f "tokens=1,2,3 delims=." %%a in ("%_VA%") do set /a "_A1=%%a,_A2=%%b,_A3=%%c"
for /f "tokens=1,2,3 delims=." %%a in ("%_VB%") do set /a "_B1=%%a,_B2=%%b,_B3=%%c"
if !_A1! GTR !_B1! exit /b 0
if !_A1! LSS !_B1! exit /b 1
if !_A2! GTR !_B2! exit /b 0
if !_A2! LSS !_B2! exit /b 1
if !_A3! GEQ !_B3! exit /b 0
exit /b 1

REM ---------------------------------------------------------------------------
REM :install_git <lib_name> <required_ver> <dir_name> <url>
REM   Skip if the lib is already installed at >= required version. Otherwise
REM   wipe and re-clone. With --reset the libraries dir is wiped upfront so
REM   the version check always fails and every lib reinstalls fresh.
REM ---------------------------------------------------------------------------
:install_git
set "_NAME=%~1"
set "_REQ=%~2"
set "_DIR=%~3"
set "_URL=%~4"
call :get_installed_version "%_DIR%"
if defined INSTALLED_VER (
    call :version_gte "!INSTALLED_VER!" "%_REQ%"
    if !ERRORLEVEL! == 0 (
        echo [OK] %_NAME% !INSTALLED_VER! already installed.
        exit /b 0
    )
    echo [UPDATE] %_NAME% !INSTALLED_VER! ^< %_REQ%, updating...
) else (
    echo [INSTALL] %_NAME% not found, cloning %_REQ%...
)
if exist "%ARDUINO_LIBS%\%_DIR%" rmdir /S /Q "%ARDUINO_LIBS%\%_DIR%"
git clone --depth 1 "%_URL%" "%ARDUINO_LIBS%\%_DIR%"
if %ERRORLEVEL% NEQ 0 echo   WARNING: git clone failed for %_NAME%.
echo.
exit /b 0

REM ---------------------------------------------------------------------------
REM :install_git_crypto <lib_name> <required_ver>
REM   Crypto lives inside the arduinolibs monorepo - needs special handling.
REM   Skip if already installed at >= required version, otherwise clone fresh.
REM ---------------------------------------------------------------------------
:install_git_crypto
set "_NAME=%~1"
set "_REQ=%~2"
call :get_installed_version "Crypto"
if defined INSTALLED_VER (
    call :version_gte "!INSTALLED_VER!" "%_REQ%"
    if !ERRORLEVEL! == 0 (
        echo [OK] %_NAME% !INSTALLED_VER! already installed.
        exit /b 0
    )
    echo [UPDATE] %_NAME% !INSTALLED_VER! ^< %_REQ%, updating...
) else (
    echo [INSTALL] %_NAME% not found, cloning %_REQ%...
)
set "TMPDIR=%TEMP%\arduinolibs_setup_%RANDOM%"
git clone --depth 1 https://github.com/rweather/arduinolibs "%TMPDIR%"
if %ERRORLEVEL% == 0 (
    if exist "%ARDUINO_LIBS%\Crypto" rmdir /S /Q "%ARDUINO_LIBS%\Crypto"
    xcopy /E /I /Q /Y "%TMPDIR%\libraries\Crypto" "%ARDUINO_LIBS%\Crypto\" >nul
    rmdir /S /Q "%TMPDIR%"
    echo   OK
) else (
    echo   WARNING: Failed to clone Crypto.
    echo   Install manually from Arduino Library Manager: "Crypto" by Rhys Weatherley
    if exist "%TMPDIR%" rmdir /S /Q "%TMPDIR%"
)
echo.
exit /b 0
