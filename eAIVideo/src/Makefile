GS_LINK = -isystem ${SDKTARGETSYSROOT}/usr/include/gstreamer-1.0 \
	  		  -isystem ${SDKTARGETSYSROOT}/usr/include/glib-2.0 \
	  		  -isystem ${SDKTARGETSYSROOT}/usr/lib64/glib-2.0/include \
	  		  -isystem  ${PWD}\
	  		  -isystem ${PWD}/yolo \
			  -lgstreamer-1.0

BSP_SDK_FLAG = \
			  -ljpeg -lwebp -ltiff -lz -ltbb -lgtk-3 -lpng16 -lgdk-3 -lcairo  \
			  -llzma -lrt -lcairo-gobject \
			  -lxkbcommon -lwayland-cursor -lwayland-egl -lwayland-client -lepoxy \
			  -lfribidi -lharfbuzz -lfontconfig \
			  -lglib-2.0 -lgobject-2.0 -lgdk_pixbuf-2.0 -lgmodule-2.0 -lpangocairo-1.0 \
			  -latk-1.0 -lgio-2.0 -lpango-1.0 -lfreetype -lpixman-1 -luuid -lpcre \
			  -lmount -lresolv -lexpat -lpangoft2-1.0 -lblkid 

SRC=eAIVideo.cpp \
    eAI_Stream.cpp \
    eAI_AppSink.cpp \
    gstutils.cpp \
    yolo/yolo_inf.cpp \
    yolo/yolov2_post.cpp \
    yolo/image.cpp 

all: eAIVideo_yolov2 \
     eAIVideo_tinyv2 \
     eAIVideo_yolov3 \
     eAIVideo_tinyv3 
     
eAIVideo_yolov2: eAIVideo.cpp
	${CXX} -std=c++14 ${SRC} \
	-DYOLOV2 \
	${GS_LINK} ${BSP_SDK_FLAG} \
	-lpthread -O2 -ldl ${LDFLAGS} -o ../exe/eAIVideo_yolov2	

eAIVideo_tinyv2: eAIVideo.cpp
	${CXX} -std=c++14 ${SRC} \
	-DTINYYOLOV2 \
	${GS_LINK} ${BSP_SDK_FLAG} \
	-lpthread -O2 -ldl ${LDFLAGS} -o ../exe/eAIVideo_tinyv2

eAIVideo_yolov3: eAIVideo.cpp
	${CXX} -std=c++14 ${SRC} \
	-DYOLOV3 \
	${GS_LINK} ${BSP_SDK_FLAG} \
	-lpthread -O2 -ldl ${LDFLAGS} -o ../exe/eAIVideo_yolov3	

eAIVideo_tinyv3: eAIVideo.cpp
	${CXX} -std=c++14 ${SRC} \
	-DTINYYOLOV3 \
	${GS_LINK} ${BSP_SDK_FLAG} \
	-lpthread -O2 -ldl ${LDFLAGS} -o ../exe/eAIVideo_tinyv3
	
clean:
	rm -rf *.o ../exe/eAIVideo*       
