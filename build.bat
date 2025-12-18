@echo off
echo Stopping any running instances of game.exe...
taskkill /F /IM game.exe >nul 2>&1

echo Building SFML Game...
g++ -g "game.cpp" -o "game.exe" -I"C:\Users\ILLEGEAR\Downloads\SFML-3.0.2-windows-gcc-14.2.0-mingw-64-bit\SFML-3.0.2\include" -L"C:\Users\ILLEGEAR\Downloads\SFML-3.0.2-windows-gcc-14.2.0-mingw-64-bit\SFML-3.0.2\lib" -lsfml-graphics -lsfml-window -lsfml-system -mconsole

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo Build successful! Running game...
game.exe
pause