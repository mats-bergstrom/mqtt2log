######################### -*- Mode: Makefile-Gmake -*- ########################
## Copyright (C) 2018, Mats Bergstrom
## $Id$
## 
## File name       : Makefile
## Description     : to build mqtt2log
## 
## Author          : Mats Bergstrom
## Created On      : Tue Oct 30 18:03:41 2018
## 
###############################################################################

CC = gcc
CFLAGS = -Wall -pedantic-errors -I../cfgf -g
LDLIBS = -lmosquitto -lcfgf
LDFLAGS = -L../cfgf

IBIN	= /usr/local/bin
ILOG	= /var/local/mqtt2log
SYSTEMD_DIR = /lib/systemd/system

BINARIES = mqtt2log
SYSTEMD_FILES = mqtt2log.service


all: mqtt2log

mqtt2log: mqtt2log.o

mqtt2log.o: mqtt2log.c



.PHONY: clean uninstall install

clean:
	rm -f *.o mqtt2log *~ *.log .*~

uninstall:
	cd $(SYSTEMD_DIR); rm $(SYSTEMD_FILES)
	cd $(IBIN); rm $(BINARIES)

install:
	if [ ! -d $(IBIN) ] ; then mkdir $(IBIN); fi
	if [ ! -d $(ILOG) ] ; then mkdir $(ILOG); fi
	cp $(BINARIES) $(IBIN)
	cp $(SYSTEMD_FILES) $(SYSTEMD_DIR)

