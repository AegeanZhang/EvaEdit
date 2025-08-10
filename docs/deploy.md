# Windows打包，使用windeployqt和Inno Setup：
windeployqt --qmldir qml --dir out\deploy out\build\debug\EvaEdit.exe
windeployqt --qmldir qml --dir out\deploy out\build\release\EvaEdit.exe

iss脚本：installer\win_setup.iss_