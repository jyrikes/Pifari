@echo off
cd /d "%~dp0"
start "Pifari Tutorial" /b python -m http.server 4784 --bind 127.0.0.1 --directory dist > "%TEMP%\pifari-tutorial-http.log" 2>&1
start "" http://127.0.0.1:4784/estudo.html
