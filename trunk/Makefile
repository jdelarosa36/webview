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
	install webview $(DESTDIR)/usr/bin
