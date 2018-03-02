#####
#
INCLUDES :=../ffmpeg-3.2.4/include/ 
CFLAGS := -Wall -g -o0 -fPIC
CFLAGS += $(addprefix -I, $(INCLUDES))

#
FFMPEG_LIBS=    avdevice                        \
                avformat                        \
                avfilter                        \
                avcodec                         \
                swresample                      \
                swscale                         \
                avutil                          \


LIBDIR := ../ffmpeg-3.2.4/lib/
LIBS   := $(FFMPEG_LIBS)
LDLIBS := $(addprefix -L,$(LIBDIR)) $(addprefix -l,$(LIBS)) -lm -lpthread -ldl -lrt 

EXAMPLES=       demuxing_decoding \
				remuxing          \
				demux-into-audio-video     \



OBJS=$(addsuffix .o,$(EXAMPLES))

.phony: all clean-test clean

all: $(OBJS) $(EXAMPLES)

clean-test:
	$(RM) test*.pgm test.h264 test.mp2 test.sw test.mpg

clean: clean-test
	$(RM) $(EXAMPLES) $(OBJS)
