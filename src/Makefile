CXX:= g++
SRCS:= gsticdsobjectsummarize.cpp icdsobjectsummarize_property_parser.cpp
INCS:= gsticdsobjectsummarize.h icdsobjectsummarize_property_parser.h
LIB:=libicdsgst_dsobjectsummarize.so

NVDS_VERSION:=6.1

GST_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/gst-plugins/
LIB_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/

CFLAGS+= -fPIC -std=c++11 -DDS_VERSION=\"6.1.0\" \
	 -I ../../includes \
	 -I ../../libs/nvds_analytics -DNDEBUG

LIBS := -shared -Wl,-no-undefined


LIBS+= -L$(LIB_INSTALL_DIR) -lnvdsgst_helper -lnvdsgst_meta -lnvds_meta \
       -lnvds_dsanalytics -ldl -lpthread -lyaml-cpp \
       -Wl,-rpath,$(LIB_INSTALL_DIR)

OBJS:= $(SRCS:.cpp=.o)

PKGS:= gstreamer-1.0 gstreamer-video-1.0
CFLAGS+= $(shell pkg-config --cflags $(PKGS))
LIBS+= $(shell pkg-config --libs $(PKGS))

all: $(LIB)

%.o: %.cpp $(INCS) Makefile
	$(CXX) -c -o $@ $(CFLAGS) $<

$(LIB): $(OBJS) $(DEP) Makefile
	$(CXX) -o $@ $(OBJS) $(LIBS)

install: $(LIB)
	cp -rv $(LIB) $(GST_INSTALL_DIR)

clean:
	rm -rf $(OBJS) $(LIB)
