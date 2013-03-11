EXTRA_DIST = Doxyfile scrap_man.t2t scrap_user_guide.t2t
man_MANS = scrap.1
all: doxygen scrap.1 scrap_user_guide.html
doxygen: Doxyfile
if DOXYGENBUILD
	doxygen Doxyfile
else
	@echo "*** No Doxygen found, not building documentation."
endif
scrap.1: scrap_man.t2t
if TXT2TAGSBUILD
	txt2tags $<
else
	@echo "*** No txt2tags found, not building manual page."
endif

scrap_user_guide.html: scrap_user_guide.t2t
if TXT2TAGSBUILD
	txt2tags scrap_user_guide.t2t
else
	@echo "*** No txt2tags found, not building user guide."
endif
clean:
	rm -rf doxygen/
	rm -f scrap.1
	rm -f scrap_user_guide.html

