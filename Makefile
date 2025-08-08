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
MAIN_AR:=csptp.a
LIBSYS_SO:=libsys/libsys.so
LIBSYS_H:=libsys/libsys.h
UTEST:=utest/utest
D_FILES:=$(wildcard src/*.d utest/*.d)

include version

CFLAGS+=-MT $@ -MMD -MP -MF $(basename $@).d
CFLAGS+=-I. -Wall -std=gnu11 -g -DVERSION=\"$(maj_ver).$(min_ver)\"

$(MAIN_AR): $(OBJS)
	$(RM) $@
	$(AR) cr $@ $^
	ranlib $@

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
override CXXFLAGS+=$(GFLAGS) -I.
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
USE:=$(USE_PHONY) $(MAIN_AR) $(UTEST) $(LIBSYS_SO) $(UOBJS) $(OBJS)
NONPHONY_TGT:=$(firstword $(filter-out $(USE),$(MAKECMDGOALS)))
ifneq ($(NONPHONY_TGT),)
$(eval $(call phony,$(NONPHONY_TGT)))
GTEST_FILTERS:=--gtest_filter=*$(NONPHONY_TGT)*
endif

.PHONY: $(USE_PHONY)

utest: $(UTEST) $(LIBSYS_SO)
	LD_PRELOAD=./$(LIBSYS_SO) $(UTEST) $(GTEST_FILTERS)
