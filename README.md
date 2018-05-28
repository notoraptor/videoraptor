# Executable videoraptor`
Compilation command (on Windows):
```batch
g++ *.hpp *.cpp core\*.hpp core\*.cpp -o .local\videoraptor -I . -I ..\ffmpeg-4.0-win64-dev\include -L ..\ffmpeg-4.0-win64-dev\lib -lavcodec -lavformat -lavutil -lswscale -O3
```
Usage:
- `videoraptor <videoFilename>`
- `videoraptor <videoFilename> <thumbnailFolder> <thumbnailNameWithoutExtension>`
- `videoraptor <txtFilename>`
  - Each line in TXT file must describe one of basic commands, with arguments separated by tabs. Example:
    - `<videoFilename>`
    - `<videoFilename>\t<thumbnailFolder>\t<thumbnailNameWithoutExtension>`

# Library `videoRaptorBatch`
```
cd .local

g++ -c ..\videoRaptorBatch.hpp ..\videoRaptorBatch.cpp ..\core\*.h ..\core\*.hpp ..\core\*.cpp -I .. -I ..\..\ffmpeg-4.0-win64-dev\include -L ..\..\ffmpeg-4.0-win64-dev\lib -lavcodec -lavformat -lavutil -lswscale -O3

g++ -shared -o videoRaptorBatch.dll *.o -fPIC -I .. -I ..\..\ffmpeg-4.0-win64-dev\include -L ..\..\ffmpeg-4.0-win64-dev\lib -lavcodec -lavformat -lavutil -lswscale -O3
```