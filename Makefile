CC=gcc
CFLAGS=-c -g -Wall -std=gnu1x -O3 -I/opt/local/include
LDFLAGS=-L/opt/local/lib -lm -lpng
SOURCES=blockCompressionCommon.c\
        colorSpaceReduction.c\
        dxtc/DXTC.c\
        dxtc/DXTC_Compress.c\
        dxtc/DXTC_CompressAlpha.c\
        dxtc/DXTC_Decompress.c\
        etc/ETC.c\
        etc/ETC_Compress.c\
        etc/ETC_Compress_Alpha.c\
        etc/ETC_Compress_Common.c\
        etc/ETC_Compress_D.c\
        etc/ETC_Compress_H.c\
        etc/ETC_Compress_I.c\
        etc/ETC_Compress_P.c\
        etc/ETC_Compress_T.c\
        etc/ETC_Decompress.c\
        lib.c\
        main.c\
        simplePNG.c\
        pvrtc/PVRTC.c\
        pvrtc/PVRTC_Compress.c\
        pvrtc/PVRTC_Decompress.c
OBJECTS=$(SOURCES:%.c=%.o)
EXECUTABLE=GLTC

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean :
	rm -f *.o dxtc/*.o etc/*.o pvrtc/*.o $(EXECUTABLE)
