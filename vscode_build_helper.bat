@echo off
cd C:\GPRS_C_SDK
echo Cleaning all
powershell -command .\build.bat clean all
echo Start building project '%1%'
echo '--------------------------'
powershell -command .\build.bat %1%