bin_PROGRAMS = scrap
scrap_SOURCES = main.c common.c enc_video.c frame_queue.c load_conf.c log.c motion.c read_conf.c sleep.c v4l.c write.c include/common.h include/device.h include/enc_video.h include/frame_queue.h include/load_conf.h include/log.h include/motion.h include/read_conf.h include/sleep.h include/v4l.h include/write.h
scrap_LDADD = ../avilib/libavi.a
AM_CFLAGS = -I./include/ -I../avilib/ -I../xpm/

../authors.h ../capture_conf.h:
	cd .. && $(MAKE) $@
../avilib/libavi.a:
	cd ../avilib/ && $(MAKE)
include/common.h: ../authors.h 
enc_video.o: ../avilib/avilib.h
main.o: ../capture_conf.h
main.o load_conf.o: include/device.h
main.o load_conf.o enc_video.o: include/enc_video.h
main.o load_conf.o: include/load_conf.h
main.o motion.o: include/motion.h
main.o sleep.o: include/sleep.h
main.o write.o: include/write.h
write.o: ../xpm/font1.xpm ../xpm/font2.xpm
main.o common.o enc_jpeg_ijg.o enc_video.o frame_queue.o load_conf.o log.o motion.o read_conf.o sleep.o v4l.o write.o: include/common.h
# main.o enc_jpeg_ijg.o load_conf.o: include/enc_jpeg.h
main.o frame_queue.o load_conf.o: include/frame_queue.h
main.o log.o: include/log.h
load_conf.o read_conf.o: include/read_conf.h
main.o load_conf.o v4l.o: include/v4l.h

