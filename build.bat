@echo off
echo Building Movie Booking System...

echo Compiling server...
g++ -std=c++11 server.cpp -o server.exe -lws2_32 -pthread
if %ERRORLEVEL% neq 0 (
    echo Server compilation failed!
    pause
    exit /b 1
)

echo Compiling client...
g++ -std=c++11 client.cpp -o client.exe -lws2_32
if %ERRORLEVEL% neq 0 (
    echo Client compilation failed!
    pause
    exit /b 1
)

echo Build successful!
echo.
echo To run:
echo 1. Run server.exe first
echo 2. Run client.exe in separate terminal
echo.
pause