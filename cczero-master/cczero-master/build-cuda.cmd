rd /s build

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
meson.py build --backend vs2015 --buildtype release ^
-Dcudnn_libdirs="C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v9.2\lib\x64","C:\dev\cuDNN\cuda\lib\x64" ^
-Dcudnn_include="C:\dev\cuDNN\cuda\include" ^
-Ddefault_library=static

pause


cd build

"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" ^
/p:Configuration=Release ^
/p:Platform=x64 ^
/p:PreferredToolArchitecture=x64 subprojects\zlib-1.2.11\subprojects@zlib-1.2.11@@z@sta.vcxproj ^
/filelogger

"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" ^
/p:Configuration=Release ^
/p:Platform=x64 ^
/p:PreferredToolArchitecture=x64 cc0@exe.vcxproj ^
/filelogger

