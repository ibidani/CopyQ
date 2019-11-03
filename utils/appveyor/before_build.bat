@echo on

set KF5_FULLVER=%KF5_VERSION%.%KF5_PATCH%
set SNORETOAST=snoretoast-%SNORETOAST_VERSION%%SNORETOAST_RELEASE%


mkdir build_deps
cd build_deps || goto :error

curl -LO https://download.kde.org/stable/snoretoast/%SNORETOAST_VERSION%/bin/%SNORETOAST%.7z || goto :error
7z x %SNORETOAST%.7z -o%SNORETOAST%
xcopy /F /E "%SNORETOAST%\*" "%QTDIR%" || goto :error
set SNORETOAST_FIX_PATH=C:\Craft\BinaryCache\windows%SNORETOAST_RELEASE%
mkdir %SNORETOAST_FIX_PATH%
xcopy /F /E "%SNORETOAST%\*" "%SNORETOAST_FIX_PATH%" || goto :error

call :kf5_build extra-cmake-modules || goto :error

call :kf5_build kconfig || goto :error

choco install -y gperf || goto :error
call :kf5_build kcodecs || goto :error

call :kf5_build kwindowsystem || goto :error
call :kf5_build kcoreaddons || goto :error
call :kf5_build knotifications || goto :error

cd ..


cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo^
 -G "%CMAKE_GENERATOR%" -A "%CMAKE_GENERATOR_ARCH%"^
 -DWITH_TESTS=ON || goto :error

:error
exit /b %errorlevel%

:kf5_build 
curl -LO https://download.kde.org/stable/frameworks/%KF5_VERSION%/%~1-%KF5_FULLVER%.zip || goto :error

cmake -E tar xf %~1-%KF5_FULLVER%.zip --format=zip
cd %~1-%KF5_FULLVER% || goto :error

sed -i "s/TARGET_FILE:kconf_update/TARGET_FILE_NAME:KF5::kconf_update/" src/kconf_update/CMakeLists.txt

cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=RelWithDebInfo^
 -G "%CMAKE_GENERATOR%" -A "%CMAKE_GENERATOR_ARCH%"^
 -DCMAKE_INSTALL_PREFIX="%QTDIR%" || goto :error

cmake --build build/ --target install || goto :error

cd ..
exit /b 0
