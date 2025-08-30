# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com>
#
# Makefile Create CSPTP service and client
#
# @author Erez Geva <ErezGeva2@@gmail.com>
# @copyright © 2025 Erez Geva
#
###############################################################################

SRCS:=$(wildcard src/*.c)
OBJS:=$(SRCS:%.c=%.o)
HDRS:=$(wildcard src/*.h)
ALL:=csptp_service csptp_client
TOBJS:=src/service.o src/client.o
USE:=$(TOBJS) $(ALL) config.h
MAIN_AR:=csptp.a
LIBSYS_SO:=libsys/libsys.so
LIBSYS_H:=libsys/libsys.h
UTEST:=utest/utest
D_FILES:=$(wildcard src/*.d utest/*.d)

include version

BFLAGS:= -I. -Wall -std=gnu11 -g -DVERSION=\"$(maj_ver).$(min_ver)\" -include config.h
override CFLAGS+= $(BFLAGS) -MT $@ -MMD -MP -MF $(basename $@).d

all: $(ALL)

config.h:
	tools/probe.sh

$(MAIN_AR): $(OBJS) | config.h
	$(RM) $@
	$(AR) cr $@ $^
	ranlib $@

$(TOBJS): config.h
	printf 'int main(int argc,char**argv){return %s(argc,argv);}' \
	 "$(basename $(@F))_main" |\
	$(CC) $(BFLAGS) -include src/main.h -c -x c - -o $@

csptp_service: src/service.o $(MAIN_AR)
	$(CC) -g $^ -o $@
csptp_client: src/client.o $(MAIN_AR)
	$(CC) -g $^ -o $@

format: $(SRCS) $(HDRS)
	astyle --project=none --options=astyle.opt $^

tags:
	ctags -R

clean:
	$(RM) $(wildcard utest/*.o) $(D_FILES) $(OBJS) $(UTEST) $(LIBSYS_SO)

include $(D_FILES)

GFLAGS:=-include gtest/gtest.h -DGTEST_HAS_PTHREAD=1
utest/main_.o:
	printf 'int main(int argc,char**argv)%s%s'\
	'{initLibSys();::testing::InitGoogleTest(&argc,argv);'\
	'return RUN_ALL_TESTS();}' |\
	$(CXX) $(GFLAGS) -include $(LIBSYS_H) -c -x c++ - -o $@

$(LIBSYS_SO): libsys/libsys.cpp $(LIBSYS_H)
	$(CXX) -shared -fPIC -include $(LIBSYS_H) -o $@ $< -ldl

override CXXFLAGS+=-MT $@ -MMD -MP -MF $(basename $@).d
override CXXFLAGS+=$(GFLAGS) -include config.h -I.
USRCS:=$(wildcard utest/*.cpp)
UOBJS:=utest/main_.o $(USRCS:%.cpp=%.o)
$(UTEST): $(UOBJS) $(MAIN_AR) $(LIBSYS_SO)
	$(CXX) $^ -o $@ -lgtest -lpthread

define phony
.PHONY: $1
$1:
	@:

endef

USE_PHONY:=format tags clean utest
USE+=$(USE_PHONY) $(MAIN_AR) $(UTEST) $(LIBSYS_SO) $(UOBJS) $(OBJS)
NONPHONY_TGT:=$(firstword $(filter-out $(USE),$(MAKECMDGOALS)))
ifneq ($(NONPHONY_TGT),)
$(eval $(call phony,$(NONPHONY_TGT)))
GTEST_FILTERS:=--gtest_filter=*$(NONPHONY_TGT)*
endif

.PHONY: $(USE_PHONY)

utest: $(UTEST) $(LIBSYS_SO)
	LD_PRELOAD=./$(LIBSYS_SO) $(UTEST) $(GTEST_FILTERS)
