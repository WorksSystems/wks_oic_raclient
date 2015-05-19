.PHONY: all clean

BUILDPATH = ../build
OBJPATH = $(BUILDPATH)/obj
BINPATH = $(BUILDPATH)/bin
LIBPATH = $(BUILDPATH)/libs

CFLAGS = -O -Wall -fdata-sections -Wl,--gc-sections -Wl,-s -fno-exceptions -std=gnu99 $(INC_DIRS) 
DEFS = -DTB_LOG

SRCPATH = ../source
INCLUDES = -I../include/ -I../include/iotivity/resource/csdk/stack/include/ -I../include/iotivity/resource/csdk/ocsocket/include/ -I../include/libstrophe/ -I../include/libstrophe/src/ -I../include/iotivity/extlibs/cjson/ -I../include/iotivity/resource/csdk/logger/include/ -I../include/iotivity/resource/oc_logger/include/
VPATH = $(SRCPATH)
SRCS=$(wildcard $(SRCPATH)/*.c) 

HEADERS=$(wildcard $(HDRPATH)/*.h)
NOTDIRSRCS=$(notdir $(SRCS))
OBJS = $(patsubst %.c,$(OBJPATH)/%.o,$(NOTDIRSRCS))


#LIBS = $(LIBPATH)/liboctbstack.a $(LIBPATH)/libstrophe.a $(LIBPATH)/libssl.a $(LIBPATH)/libcrypto.a  $(LIBPATH)/libexpat.a  $(LIBPATH)/libcoap.a  $(LIBPATH)/oc_logger.a   -lpthread  -lm -lresolv
LIBS = $(LIBPATH)/liboctbstack.a $(LIBPATH)/libstrophe.a  $(LIBPATH)/libexpat.a  $(LIBPATH)/libcoap.a  $(LIBPATH)/oc_logger.a -lssl -lcrypto -lpthread -lm -lresolv

all: wks_raclient
      
wks_raclient: $(OBJS)
	@mkdir -p $(BINPATH)
	$(CC) -o $(BINPATH)/$@ $^ $(LIBS)
	@echo Compile WKS-OIC-RACLIENT successful....

$(OBJS): $(OBJPATH)/%.o: %.c
	@mkdir -p $(OBJPATH)
	$(CC) $(CFLAGS) -c -o $@  $< $(INCLUDES) $(DEFS)

clean:
	rm -rf $(BINPATH) $(OBJPATH)
