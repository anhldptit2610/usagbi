@echo off
setlocal

:: --- CONFIGURATION ---
:: Path to your C++ executable (use quotes if path has spaces)
set "EXE_PATH=..\build\tests\cpu_instructions\cpu_instr_test.exe"

:: Folder containing the input files
set "INPUT_FOLDER=..\tests\cpu_instructions\sm83\v1\"

:: File extension to look for (use *.* for all files, or *.txt, *.json, etc.)
set "FILE_MASK=*.*"
:: ---------------------

echo Starting batch execution...
echo ------------------------------------------

:: Loop through files in the folder
for %%f in ("%INPUT_FOLDER%\%FILE_MASK%") do (
    echo Processing file: %%~nxf
    
    :: Execute the program passing the full file path as an argument
    "%EXE_PATH%" "%%f"

    if errorlevel 1 (
        echo [FAIL] Test failed on %%~nxf file
        exit
    ) else (
        echo [OK] Success on %%~nxf
    )
    
    echo ------------------------------------------
)

echo All files processed.
exit
