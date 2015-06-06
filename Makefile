# Makefile for Linux and Maemo
# Copyright (C) 2008-2011 Axel Sommerfeldt (axel.sommerfeldt@f-m.fm)
#
# --------------------------------------------------------------------
#
# This file is part of the Leia application.
#
# Leia is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Leia is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Leia.  If not, see <http://www.gnu.org/licenses/>.
#
# --------------------------------------------------------------------
#
# Available actions:
# make         - will build Leia
# make leiads  - will build Leia, too
# make run     - will run Leia
# make install - will install Leia
# make deb     - will build a distribution package of Leia
# make tar     - will archive the directory content into ../leiads.tar.gz
# make clean   - cleanup
#
# --------------------------------------------------------------------

# Log level:
# 0 = upnp_error + upnp_critical
# 1 = + upnp_assert + upnp_warning
# 2 = + upnp_message + TRACE
# 3 = + upnp_debug
# 4 = + UPnP: SOAP & GENA
# 5 = + UPnP: send/recv headers
# 6 = + UPnP: send/recv bodies
LOGLEVEL = 0

# If we run inside a Scratchbox environment, we will make a Maemo binary/package by default
ifdef SBOX_UNAME_MACHINE
MAEMO = 1
else
MAEMO = 0
endif

# Osso service, needed for Maemo
OSSO_SERVICE = de.sommerfeldt.leiads

# Directories
BIN = $(DESTDIR)/usr/bin
SHARE = $(DESTDIR)/usr/share
APPLICATIONS = $(DESTDIR)/usr/share/applications
DBUS_SERVICES = $(DESTDIR)/usr/share/dbus-1/services
ICONS = $(DESTDIR)/usr/share/icons/hicolor

# Compiler flags
CFLAGS = `pkg-config gtk+-2.0 gthread-2.0 --cflags --libs` \
	-Wall -pedantic -DLOGLEVEL=$(LOGLEVEL)
ifneq ($(MAEMO),0)
CFLAGS += `pkg-config hildon-1 hildon-fm-2 conic libosso --cflags --libs` \
	-DMAEMO -DOSSO_SERVICE=\"$(OSSO_SERVICE)\"
endif
ifndef SBOX_UNAME_MACHINE
CFLAGS += -Wno-overlength-strings -Wno-format-security
endif
-include ../lastfm/Makefile.inc
-include ../radiotime/Makefile.inc
CFLAGS += -DRADIOTIME_LINN_DS=1

# Source files
SRC = src/leia-ds.c \
	src/browser.c src/playlist.c src/playlist_file.c src/now_playing.c \
	src/about.c src/info.c src/lastfm.c src/radiotime.c src/metadata.c \
	src/select_renderer.c src/settings.c src/transport.c src/volume.c \
	src/intl.c src/keybd.c src/miscellaneous.c src/debug.c src/md5.c \
	src/princess/media_renderer.c \
	src/princess/linn_source.c src/princess/linn_preamp.c \
	src/princess/upnp_media_renderer.c src/princess/upnp_media_server.c \
	src/princess/upnp.c src/princess/upnp_list.c src/princess/upnp_xml.c src/princess/upnp_private.c

# Header files
HDR = src/leia-ds.h \
	src/princess/media_renderer.h \
	src/princess/linn_source.h src/princess/linn_preamp.h \
	src/princess/upnp_media_renderer.h src/princess/upnp_media_server.h \
	src/princess/upnp.h src/princess/upnp_private.h

# make leiads
leiads: $(SRC) $(HDR)
	gcc -o leiads $(SRC) $(CFLAGS)
	strip leiads

# make all
.PHONY:	all
all:	leiads

# make clean
.PHONY: clean
clean:
	rm -fr leiads debian/leiads core

# make run
.PHONY: run
run:	leiads
ifdef SBOX_UNAME_MACHINE
	run-standalone.sh ./leiads share=./share
else
	./leiads share=./share
endif

# make tar
.PHONY: tar
tar:	clean
	-rm -f ../leiads.tar.gz
	tar cvfz ../leiads.tar.gz *

# make install
.PHONY: install
install: leiads
ifeq ($(MAEMO),0)
	install -d $(BIN) $(SHARE)/leiads $(APPLICATIONS) \
	           $(ICONS)/16x16/apps $(ICONS)/26x26/apps $(ICONS)/32x32/apps \
	           $(ICONS)/40x40/apps $(ICONS)/48x48/apps $(ICONS)/64x64/apps \
	           $(ICONS)/scalable/apps
	install -m 755 ./leiads $(BIN)
	install -m 644 ./share/* $(SHARE)/leiads
	install -m 644 ./leiads.desktop $(APPLICATIONS)
	install -m 644 ./icons/16x16/leiads.png $(ICONS)/16x16/apps
	install -m 644 ./icons/26x26/leiads.png $(ICONS)/26x26/apps
	install -m 644 ./icons/32x32/leiads.png $(ICONS)/32x32/apps
	install -m 644 ./icons/40x40/leiads.png $(ICONS)/40x40/apps
	install -m 644 ./icons/48x48/leiads.png $(ICONS)/48x48/apps
	install -m 644 ./icons/64x64/leiads.png $(ICONS)/64x64/apps
	install -m 644 ./icons/scalable/leiads.png $(ICONS)/scalable/apps
else
	install -d $(BIN) $(SHARE)/leiads $(APPLICATIONS)/hildon $(DBUS_SERVICES) \
	           $(ICONS)/26x26/hildon $(ICONS)/40x40/hildon $(ICONS)/scalable/hildon
	install -m 755 ./leiads $(BIN)
	install -m 644 ./share/* $(SHARE)/leiads
	install -m 644 ./leiads.desktop $(APPLICATIONS)/hildon
	install -m 644 ./$(OSSO_SERVICE).service $(DBUS_SERVICES)
	install -m 644 ./icons/26x26/leiads.png $(ICONS)/26x26/hildon
	install -m 644 ./icons/40x40/leiads.png $(ICONS)/40x40/hildon
	install -m 644 ./icons/scalable/leiads.png $(ICONS)/scalable/hildon
endif

# make deb
.PHONY: deb
deb:	clean
	dpkg-buildpackage -rfakeroot

