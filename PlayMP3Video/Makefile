GS_LINK = -isystem ${SDKTARGETSYSROOT}/usr/include/gstreamer-1.0 \
	  		  -isystem ${SDKTARGETSYSROOT}/usr/include/glib-2.0 \
	  		  -isystem ${SDKTARGETSYSROOT}/usr/lib64/glib-2.0/include \
	  		  -isystem ${PWD} \
			  -lgstreamer-1.0

BSP_SDK_FLAG = \
			  -ljpeg -lwebp -ltiff -lz -ltbb -lgtk-3 -lpng16 -lgdk-3 -lcairo  \
			  -llzma -lrt -lcairo-gobject \
			  -lxkbcommon -lwayland-cursor -lwayland-egl -lwayland-client -lepoxy \
			  -lfribidi -lharfbuzz -lfontconfig \
			  -lglib-2.0 -lgobject-2.0 -lgdk_pixbuf-2.0 -lgmodule-2.0 -lpangocairo-1.0 \
			  -latk-1.0 -lgio-2.0 -lpango-1.0 -lfreetype -lpixman-1 -luuid -lpcre \
			  -lmount -lresolv -lexpat -lpangoft2-1.0 -lblkid \
			  
all: PlayMP3video \
     PlayMP3video_vga
     
PlayMP3video: PlayMP3video.cpp
	${CXX} -std=c++14 PlayMP3video.cpp gstDebug.cpp \
	${GS_LINK} ${BSP_SDK_FLAG} \
	-lpthread -O2 -ldl ${LDFLAGS} -o bin/PlayMP3video	

PlayMP3video_vga: PlayMP3video.cpp
	${CXX} -std=c++14 PlayMP3video.cpp gstDebug.cpp \
	-DDISPLAY_VGA \
	${GS_LINK} ${BSP_SDK_FLAG} \
	-lpthread -O2 -ldl ${LDFLAGS} -o bin/PlayMP3video_vga	
clean:
	rm -rf *.o bin/*
