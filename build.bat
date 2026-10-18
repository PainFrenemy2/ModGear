@echo off
rem ModGear - сборка под Windows.
rem Ставит Git, CMake и Visual Studio 2022 Build Tools через winget (если их нет),
rem собирает плагин и кладёт готовые файлы в папку out.
rem   build.bat        - всё (ModGear и ModGearAlfa, SA-MP и open.mp)
rem   build.bat full   - только ModGear
rem   build.bat alfa   - только ModGearAlfa
rem   build.bat samp   - только SA-MP
rem   build.bat omp    - только open.mp
chcp 65001 >nul
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
title ModGear build

set "ONLY_EDITION="
set "ONLY_PLATFORM="
for %%A in (%*) do (
	if /i "%%A"=="full" set "ONLY_EDITION=full"
	if /i "%%A"=="alfa" set "ONLY_EDITION=alfa"
	if /i "%%A"=="samp" set "ONLY_PLATFORM=samp"
	if /i "%%A"=="omp"  set "ONLY_PLATFORM=omp"
)

echo.
echo === 1/3 Проверяю программы ===

where winget >nul 2>nul
if errorlevel 1 (
	echo [ОШИБКА] Нет winget. Установи "App Installer" из Microsoft Store и запусти снова.
	goto :fail
)

where git >nul 2>nul
if errorlevel 1 (
	if exist "%ProgramFiles%\Git\cmd\git.exe" (
		set "PATH=%ProgramFiles%\Git\cmd;%PATH%"
	) else (
		echo Ставлю Git...
		winget install --id Git.Git -e --silent --accept-package-agreements --accept-source-agreements
		set "PATH=%ProgramFiles%\Git\cmd;%PATH%"
	)
)
where git >nul 2>nul || (echo [ОШИБКА] Git не установился. & goto :fail)

where cmake >nul 2>nul
if errorlevel 1 (
	if exist "%ProgramFiles%\CMake\bin\cmake.exe" (
		set "PATH=%ProgramFiles%\CMake\bin;%PATH%"
	) else (
		echo Ставлю CMake...
		winget install --id Kitware.CMake -e --silent --accept-package-agreements --accept-source-agreements
		set "PATH=%ProgramFiles%\CMake\bin;%PATH%"
	)
)
where cmake >nul 2>nul || (echo [ОШИБКА] CMake не установился. & goto :fail)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
	echo Ставлю Visual Studio 2022 Build Tools с C++ ^(около 2-3 ГБ, 10-20 минут^)...
	winget install --id Microsoft.VisualStudio.2022.BuildTools -e --silent --accept-package-agreements --accept-source-agreements --override "--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
	if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
)
if not defined VSPATH (
	echo [ОШИБКА] Не нашёл Visual Studio 2022 с C++. Перезагрузи ПК и запусти build.bat ещё раз.
	goto :fail
)

echo.
echo === 2/3 Библиотеки open.mp ===
if /i not "%ONLY_PLATFORM%"=="samp" (
	if exist ".git" (
		git submodule update --init --recursive || goto :fail
	) else (
		if not exist "lib\omp-sdk\CMakeLists.txt" call :clone https://github.com/openmultiplayer/open.mp-sdk lib\omp-sdk 3dc9cf854c2a002a683baf50d5845f396c634e2c || goto :fail
		if not exist "lib\omp-net\CMakeLists.txt" call :clone https://github.com/openmultiplayer/open.mp-network lib\omp-net dc3eac9d5dc30f96edcf4e7e64f33d8c241d49ff || goto :fail
	)
)

echo.
echo === 3/3 Сборка ===
if exist out rmdir /s /q out
for %%E in (full alfa) do (
	for %%P in (samp omp) do (
		set "SKIP=0"
		if defined ONLY_EDITION if /i not "%%E"=="!ONLY_EDITION!" set "SKIP=1"
		if defined ONLY_PLATFORM if /i not "%%P"=="!ONLY_PLATFORM!" set "SKIP=1"
		if "!SKIP!"=="0" call :build %%E %%P || goto :fail
	)
)

echo.
echo Готово, файлы лежат в папке out.
pause
exit /b 0

:build
set "ED=%~1"
set "PL=%~2"
set "NAME=ModGear"
if /i "%ED%"=="alfa" set "NAME=ModGearAlfa"
echo.
echo --- %NAME% (%PL%) ---
cmake -S . -B "build\%ED%-%PL%" -G "Visual Studio 17 2022" -A Win32 -DMODGEAR_PLATFORM=%PL% -DMODGEAR_EDITION=%ED% -DCMAKE_POLICY_VERSION_MINIMUM=3.5 || exit /b 1
cmake --build "build\%ED%-%PL%" --config Release --parallel || exit /b 1
set "DLL="
for /r "build\%ED%-%PL%\server" %%F in (%NAME%.dll) do if exist "%%F" set "DLL=%%F"
if not defined DLL (echo [ОШИБКА] Не нашёл %NAME%.dll после сборки & exit /b 1)
if /i "%PL%"=="samp" (
	mkdir "out\%NAME%-SAMP\plugins" "out\%NAME%-SAMP\pawno\include" 2>nul
	copy /y "!DLL!" "out\%NAME%-SAMP\plugins\" >nul
	copy /y "include\%NAME%.inc" "out\%NAME%-SAMP\pawno\include\" >nul
	if "%NAME%"=="ModGear" copy /y "include\ModGear_*.inc" "out\%NAME%-SAMP\pawno\include\" >nul
) else (
	mkdir "out\%NAME%-openmp\components" "out\%NAME%-openmp\qawno\include" 2>nul
	copy /y "!DLL!" "out\%NAME%-openmp\components\" >nul
	copy /y "include\%NAME%.inc" "out\%NAME%-openmp\qawno\include\" >nul
	if "%NAME%"=="ModGear" copy /y "include\ModGear_*.inc" "out\%NAME%-openmp\qawno\include\" >nul
)
exit /b 0

:clone
echo Качаю %~2...
if exist "%~2" rmdir /s /q "%~2"
git clone --recursive "%~1" "%~2" || exit /b 1
git -C "%~2" checkout -q %~3 || exit /b 1
git -C "%~2" submodule update --init --recursive || exit /b 1
exit /b 0

:fail
echo.
echo [ОШИБКА] Сборка не удалась, смотри текст выше.
pause
exit /b 1
