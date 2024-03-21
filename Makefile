all: pngparam.exe
pngparam.exe: pngparam.cpp
	cl /std:c++latest /O2 /EHsc -I rapidjson/include pngparam.cpp
