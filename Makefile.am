EXTRA_DIST = config xpm
SUBDIRS = avilib src doc

all: authors.h capture_conf.h

authors.h: AUTHORS
	echo -n "#define AUTHORS \"" > $@
	sed s/\\\'/\\\\n/g < $< | sed s/\"/\\\\\"/g | tr -d \\n >> $@
	echo \" >> $@
capture_conf.h: config/capture.conf
	echo -n "#define CAPTURE_CONF \"" > $@
	sed s/\\\'/\\\\n/g < $< | sed s/\"/\\\\\"/g | tr -d \\n >> $@
	echo \" >> $@
clean:
	rm -f authors.h capture_conf.h
