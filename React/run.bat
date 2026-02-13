@echo off
chcp 65001 >nul
cls
echo ========================================
echo React Learning System - Day 1
echo ========================================
echo.

:menu
echo Please select a learning material:
echo.
echo [1] 01. Variables (30 min)
echo [2] 02. Data Types (30 min)
echo [3] 03. Template Strings (20 min)
echo [4] 04. Practice (40 min)
echo [5] Run All Files
echo [6] Open Browser Page
echo [0] Exit
echo.
set /p choice=Enter option (0-6):

if "%choice%"=="1" goto file1
if "%choice%"=="2" goto file2
if "%choice%"=="3" goto file3
if "%choice%"=="4" goto file4
if "%choice%"=="5" goto all
if "%choice%"=="6" goto browser
if "%choice%"=="0" goto end
echo Invalid option, please try again
echo.
goto menu

:file1
echo.
echo ========================================
echo Running: 01. Variables
echo ========================================
echo.
node docs/react_basics/01_js_variables.jsx
echo.
echo ========================================
echo Completed!
echo ========================================
echo.
pause
goto menu

:file2
echo.
echo ========================================
echo Running: 02. Data Types
echo ========================================
echo.
node docs/react_basics/02_js_datatypes.jsx
echo.
echo ========================================
echo Completed!
echo ========================================
echo.
pause
goto menu

:file3
echo.
echo ========================================
echo Running: 03. Template Strings
echo ========================================
echo.
node docs/react_basics/03_js_template_strings.jsx
echo.
echo ========================================
echo Completed!
echo ========================================
echo.
pause
goto menu

:file4
echo.
echo ========================================
echo Running: 04. Practice
echo ========================================
echo.
node docs/react_basics/04_practice_hello.jsx
echo.
echo ========================================
echo Completed!
echo ========================================
echo.
pause
goto menu

:all
echo.
echo ========================================
echo Running all learning files...
echo ========================================
echo.
echo [1/4] Variables
node docs/react_basics/01_js_variables.jsx
echo.
echo [2/4] Data Types
node docs/react_basics/02_js_datatypes.jsx
echo.
echo [3/4] Template Strings
node docs/react_basics/03_js_template_strings.jsx
echo.
echo [4/4] Practice
node docs/react_basics/04_practice_hello.jsx
echo.
echo ========================================
echo All files completed!
echo ========================================
echo.
pause
goto menu

:browser
echo.
echo Opening browser page...
start docs/index.html
echo.
echo Tip: If the page does not display correctly,
echo please install Live Server plugin in VSCode
echo and right-click index.html to select "Open with Live Server"
echo.
pause
goto menu

:end
echo.
echo Thank you for using React Learning System!
echo Remember to say "Learning Complete" to generate today's log.
echo.
pause
exit
