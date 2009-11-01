# Copyright (c) 2007-2008 INdT, (c) 2009 Nokia.
#  This code example is licensed under a MIT-style license,
# that can be found in the file called "COPYING" in the package

CC = gcc

CFLAGS = -g -O0
PKGCFLAGS = `pkg-config --cflags  gtk+-2.0 \
								  hildon-1 \
								  gmodule-2.0 \
								  gconf-2.0 \
								  gthread-2.0 \
								  gstreamer-0.10 \
								  glib-2.0 \
								  gnome-vfs-2.0 \
								  libhildondesktop \
								  hildon-desktop \
								  hildon-fm-2 \
								  hildon-help \
								  libosso \
								  gnome-vfs-2.0 \
								  gstreamer-0.10 \
								  libebook-1.2` -ansi -Wall

LIBS = `pkg-config --libs gtk+-2.0 hildon-1 gthread-2.0` -lgstinterfaces-0.10

TARGET = webview

SOURCES = src/main.c src/camera.c

$(TARGET): $(SOURCES)
	$(CC) -o $(TARGET) $(SOURCES) $(CFLAGS) $(PKGCFLAGS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(TARGET)

install:
	@echo You must be root to install
	mkdir -p $(DESTDIR)/usr/bin
	install webview $(DESTDIR)/usr/bin/webview
	install cheetah $(DESTDIR)/usr/bin/cheetah
	
	mkdir -p $(DESTDIR)/usr/var/webview/config
	mkdir -p $(DESTDIR)/usr/var/webview/website
	chmod 777 $(DESTDIR)/usr/var/webview/website
	mkdir -p $(DESTDIR)/usr/var/webview/dialogs	
	mkdir -p $(DESTDIR)/usr/share/applications/hildon

	install config/webview.d $(DESTDIR)/usr/var/webview/config/webview.d
	install -m 644 website/index.htm $(DESTDIR)/usr/var/webview/website/index.htm
	install website/missing_image.png $(DESTDIR)/usr/var/webview/website/missing_image.png
	install -m 766 website/missing_image.png $(DESTDIR)/usr/var/webview/website/Picture.jpg
	install dialogs/about.xml $(DESTDIR)/usr/var/webview/dialogs/about.xml
	install dialogs/settings.xml $(DESTDIR)/usr/var/webview/dialogs/settings.xml
	install debian/webview.desktop $(DESTDIR)/usr/share/applications/hildon/webview.desktop