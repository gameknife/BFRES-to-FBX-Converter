@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\Msbuild.exe" "src\BFRESImporter\BFRESImporter.csproj" /p:configuration=Release
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\Msbuild.exe" "src\FBXExporter\FBXExporter.vcxproj" /p:configuration=release /p:platform=x64
REM python BatchProcess\RunMe.py -r
pause