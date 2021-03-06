SRCS = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o, $(SRCS))

DRIVER = libdirectfb_render.so

CFLAGS = `pkg-config --cflags directfb-internal` 
CFLAGS += `pkg-config --cflags fusion` 
CFLAGS += -I/usr/include/X11
CFLAGS += -Isrc -fPIC

LDFLAGS = `pkg-config --libs directfb-internal` 
LDFLAGS += `pkg-config --libs fusion` 
LDFLAGS += -lX11 -lXrender -lXext
LDFLAGS += -lGL -fPIC

DIRECTFB_DRIVER_PATH=`pkg-config directfb-internal --variable=moduledir`

all: build_render

install: build_render
	cp $(DRIVER) $(DIRECTFB_DRIVER_PATH)/systems
	@echo "Use system = render"
	@echo "Acuerdese tambien de poner pixelformat = ARGB"

uninstall: 
	rm -rf $(DIRECTFB_DRIVER_PATH)/systems/$(DRIVER)

clean:
	rm -rf src/*.o $(DRIVER) build_render

build_render: $(OBJS)
	echo $(OBJS)
	gcc -o $(DRIVER) $(OBJS) $(CFLAGS) $(LDFLAGS) -shared
	touch build_render
