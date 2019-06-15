# Library `videoRaptorBatch`

`md build`

`cd build`

`g++ -c ..\core\*.cpp ..\videoRaptorBatch\*.cpp ..\lib\lodepng\*.cpp  -I .. -I %CPATH% -L %LIBRARY_PATH% -lavcodec -lavformat -lavutil -lswscale -O3`

`g++ -shared -o videoRaptorBatch.dll *.o -fPIC -I .. -I %CPATH% -L %LIBRARY_PATH% -lavcodec -lavformat -lavutil -lswscale -O3`


# executable `test`

`cd cmake-build-release`

`g++ ..\test.cpp ..\videoRaptorBatch\videoRaptorBatch.* ..\lib\lodepng\lodepng.* ..\lib\utf\utf.hpp ..\core\* -o test -I .. -I %CPATH% -L %LIBRARY_PATH% -lavcodec -lavformat -lavutil -lswscale -O3`
