SUBDIRS = src

doc_DATA = AUTHORS COPYING README

PNV = $(PACKAGE_NAME)-$(PACKAGE_VERSION)

debian-package: all
	rm -rf "$(PNV)" "$(PACKAGE_NAME)_$(PACKAGE_VERSION)_$(DEBIAN_ARCH).deb"
	mkdir -p "$(PNV)/DEBIAN"
	cp debian/control "$(PNV)/DEBIAN"
	make install DESTDIR="$$PWD/$(PNV)"
	fakeroot dpkg-deb -b "$(PNV)" \
	         "$(PACKAGE_NAME)_$(PACKAGE_VERSION)_$(DEBIAN_ARCH).deb"
	rm -rf "$(PNV)"

windows-package: all
	rm -rf "$(PNV)-windows" "$(PNV)-windows.zip"
	mkdir "$(PNV)-windows"
	for DOC in AUTHORS COPYING GPL3 README; do \
	  awk '{print $$0, "\r";}' "$$DOC" > "$(PNV)-windows/$$DOC.txt"; \
	done
	cp src/unnks.exe "$(PNV)-windows"
	cp src/nks-scan.exe "$(PNV)-windows"
	cp src/nks-ls-libs.exe "$(PNV)-windows"
	zip -r "$(PNV)-windows.zip" "$(PNV)-windows"
	rm -rf "$(PNV)-windows"

EXTRA_DIST = GPL3 libs.conf mklibdata
DISTCLEANFILES = *~
