set ROOT=%CD%
set EDK2=%ROOT%\compile\toolchain\edk2
set VENV=%EDK2%\.venv

REM Create venv if missing
if not exist %VENV%\Scripts\activate.bat (
    py -m venv "%VENV%"
)
REM Activate venv
call "%VENV%\Scripts\activate.bat"

REM Install Python dependencies
pip install --upgrade pip
pip install -r "%EDK2%\pip-requirements.txt"

REM Run Stuart setup
stuart_setup -c "%EDK2%\.pytool\CISettings.py"

REM Run Stuart Update
stuart_update -c "%EDK2%\.pytool\CISettings.py"

set ROOT=
set EDK2=
set VENV=