msbuild %1.sln -t:Rebuild -p:Configuration=Debug -p:Platform=x64
msbuild %1.sln -t:Rebuild -p:Configuration=Release -p:Platform=x64
