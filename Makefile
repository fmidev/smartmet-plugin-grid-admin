SUBNAME = grid-admin
SPEC = smartmet-plugin-$(SUBNAME)
INCDIR = smartmet/plugins/$(SUBNAME)

REQUIRES = gdal configpp

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

# Enabling / disabling CORBA usage.

CORBA = enabled

ifeq ($(CORBA), disabled)
  CORBA_FLAGS = -DCORBA_DISABLED
else
  CORBA_INCLUDE = -I/usr/include/smartmet/grid-content/contentServer/corba/stubs
  CORBA_LIBS = -lomniORB4 -lomnithread
endif

# Compiler options

CFLAGS += -Wno-maybe-uninitialized $(CORBA_FLAGS)

DEFINES = -DUNIX -D_REENTRANT

INCLUDES += \
	-I$(includedir)/smartmet \
	-I$(includedir)/smartmet/grid-files \
	-I$(includedir)/smartmet/grid-content \
	$(CORBA_INCLUDE)


LIBS += $(PREFIX_LDFLAGS) \
	$(CORBA_LIBS) \
	-lsmartmet-grid-files \
	-lsmartmet-grid-content \
	-lsmartmet-spine \
	-lsmartmet-macgyver \
	$(REQUIRED_LIBS)

# What to install

LIBFILE = $(SUBNAME).so

# Compilation directories

vpath %.cpp $(SUBNAME)
vpath %.h $(SUBNAME)

# The files to be compiled

SRCS = $(wildcard $(SUBNAME)/*.cpp)
HDRS = $(wildcard $(SUBNAME)/*.h)
OBJS = $(patsubst %.cpp, obj/%.o, $(notdir $(SRCS)))

INCLUDES := -I$(SUBNAME) $(INCLUDES)

.PHONY: test rpm

# The rules

all: objdir $(LIBFILE)
debug: all
release: all
profile: all

configtest:
	@if [ -x "$$(command -v cfgvalidate)" ]; then cfgvalidate -v cnf/crid-admin.conf.sample; fi

$(LIBFILE): $(OBJS)
	$(CXX) $(LDFLAGS) -shared -rdynamic -o $(LIBFILE) $(OBJS) $(LIBS)
	@echo Checking $(LIBFILE) for unresolved references
	@if ldd -r $(LIBFILE) 2>&1 | c++filt | grep ^undefined\ symbol | grep -v SmartMet::Engine:: ; \
		then rm -v $(LIBFILE); \
		exit 1; \
	fi

clean:
	rm -f $(LIBFILE) *~ $(SUBNAME)/*~
	rm -rf obj

clean-install:
	rm -f $(plugindir)/$(LIBFILE)

format:
	clang-format -i -style=file $(SUBNAME)/*.h $(SUBNAME)/*.cpp test/*.cpp

install:
	@mkdir -p $(plugindir)
	$(INSTALL_PROG) $(LIBFILE) $(plugindir)/$(LIBFILE)

test:
	cd test && make test

objdir:
	@mkdir -p $(objdir)

rpm: clean $(SPEC).spec
	rm -f $(SPEC).tar.gz # Clean a possible leftover from previous attempt
	tar -czvf $(SPEC).tar.gz --exclude test --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -tb $(SPEC).tar.gz
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -MD -MF $(patsubst obj/%.o, obj/%.d, $@) -MT $@ -o $@ $<

-include $(wildcard obj/*.d)
