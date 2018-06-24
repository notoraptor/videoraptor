# Library `videoRaptorBatch`

```
cd .local
g++ -c ..\videoRaptorBatch\* ..\core\* ..\lodepng\* -I .. -I ..\..\ffmpeg-4.0-win64-dev\include -L ..\..\ffmpeg-4.0-win64-dev\lib -lavcodec -lavformat -lavutil -lswscale -O3
g++ -shared -o videoRaptorBatch.dll *.o -fPIC -I .. -I ..\..\ffmpeg-4.0-win64-dev\include -L ..\..\ffmpeg-4.0-win64-dev\lib -lavcodec -lavformat -lavutil -lswscale
```
