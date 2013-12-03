if not exist "%VS90COMNTOOLS%vsvars32.bat" then goto no2008
call "%VS90COMNTOOLS%vsvars32.bat"
pushd projects\vs2008
vcbuild glsl_optimizer.sln /rebuild "Debug32|Win32"
vcbuild glsl_optimizer.sln /rebuild "Debug64|x64"
vcbuild glsl_optimizer.sln /rebuild "Release32|Win32"
vcbuild glsl_optimizer.sln /rebuild "Release64|x64"
popd

:no2008
if not exist "%VS100COMNTOOLS%vsvars32.bat" then goto no2010
call "%VS100COMNTOOLS%vsvars32.bat"
pushd projects\vs2010
msbuild glsl_optimizer.sln /t:Rebuild /p:Configuration=Debug32   /p:Platform=Win32
msbuild glsl_optimizer.sln /t:Rebuild /p:Configuration=Debug64   /p:Platform=x64
msbuild glsl_optimizer.sln /t:Rebuild /p:Configuration=Release32 /p:Platform=Win32
msbuild glsl_optimizer.sln /t:Rebuild /p:Configuration=Release64 /p:Platform=x64
popd

:no2010
if not exist "%VS110COMNTOOLS%vsvars32.bat" then goto no2012
call "%VS110COMNTOOLS%vsvars32.bat"
pushd projects\vs2012
msbuild glsl_optimizer.sln /t:Rebuild /p:Configuration=Debug32   /p:Platform=Win32
msbuild glsl_optimizer.sln /t:Rebuild /p:Configuration=Debug64   /p:Platform=x64
msbuild glsl_optimizer.sln /t:Rebuild /p:Configuration=Release32 /p:Platform=Win32
msbuild glsl_optimizer.sln /t:Rebuild /p:Configuration=Release64 /p:Platform=x64
popd

:no2012
