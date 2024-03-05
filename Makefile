######################### -*- Mode: Makefile-Gmake -*- ########################
## Copyright (C) 2018, Mats Bergstrom
## 
## File name       : Makefile
## Description     : to build mqtt2log
## 
## Author          : Mats Bergstrom
## Created On      : Tue Oct 30 18:03:41 2018
## 
###############################################################################

CC	= gcc
CFLAGS	= -Wall -pedantic-errors -g
CPPFLAGS= -Icfgf
LDLIBS	= -lmosquitto -lcfgf
LDFLAGS = -Lcfgf

IBIN	= /usr/local/bin
ILOG	= /var/local/mqtt2log
ETCDIR	= /usr/local/etc
SYSTEMD_DIR = /lib/systemd/system

BINARIES	= mqtt2log
SYSTEMD_FILES	= mqtt2log.service
CFGS		= mqtt2log.cfg

CFGFGIT		= https://github.com/mats-bergstrom/cfgf.git

all: cfgf mqtt2log

mqtt2log: mqtt2log.o

mqtt2log.o: mqtt2log.c



.PHONY: cfgf really-clean clean uninstall install

cfgf:
	if [ ! -d cfgf ] ; then git clone $(CFGFGIT) ; fi
	cd cfgf && make

really-clean:
	rm -rf cfgf
clean:
	rm -f *.o mqtt2log *~ *.log .*~
	cd cfgf && make clean

uninstall:
	cd $(SYSTEMD_DIR); rm $(SYSTEMD_FILES)
	cd $(IBIN); rm $(BINARIES)

install:
	if [ ! -d $(IBIN) ] ; then mkdir $(IBIN); fi
	if [ ! -d $(ILOG) ] ; then mkdir $(ILOG); fi
	if [ ! -d $(ETCDIR) ] ; then mkdir $(ETCDIR); fi
	cp $(BINARIES) $(IBIN)
	cp $(SYSTEMD_FILES) $(SYSTEMD_DIR)
	cp $(CFGS) $(ETCDIR)
