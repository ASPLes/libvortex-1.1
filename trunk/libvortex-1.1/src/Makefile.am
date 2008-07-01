EXTRA_DIST = Makefile.win libvortex-1.1.def libvortex.vcproj

if ENABLE_VORTEX_LOG
INCLUDE_VORTEX_LOG=-DENABLE_VORTEX_LOG
endif

if ENABLE_POLL_SUPPORT
INCLUDE_VORTEX_POLL=-DVORTEX_HAVE_POLL=1
endif

if ENABLE_EPOLL_SUPPORT
INCLUDE_VORTEX_EPOLL=-DVORTEX_HAVE_EPOLL=1
endif

if DEFAULT_EPOLL
INCLUDE_DEFAULT_EPOLL=-DDEFAULT_EPOLL 
endif

if DEFAULT_POLL
INCLUDE_DEFAULT_POLL=-DDEFAULT_POLL 
endif

INCLUDES = -Wall -ansi -g -I$(top_srcdir) -D__COMPILING_VORTEX__ \
	$(AXL_CFLAGS) $(INCLUDE_VORTEX_LOG) $(PTHREAD_CFLAGS) \
	-DVERSION=\""$(VORTEX_VERSION)"\" \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" $(INCLUDE_VORTEX_POLL) $(INCLUDE_VORTEX_EPOLL) $(INCLUDE_DEFAULT_EPOLL) $(INCLUDE_DEFAULT_POLL)

libvortex_1_1_includedir = $(includedir)/vortex-1.1

lib_LTLIBRARIES = libvortex-1.1.la

libvortex_1_1_la_SOURCES =	\
	vortex.c \
	vortex_ctx.c \
	vortex_support.c \
	vortex_connection.c \
	vortex_listener.c  \
	vortex_handlers.c  \
	vortex_frame_factory.c \
	vortex_greetings.c \
	vortex_profiles.c \
	vortex_channel.c \
	vortex_io.c \
	vortex_reader.c \
	vortex_dtds.c \
	vortex_hash.c \
	vortex_queue.c \
	vortex_sequencer.c \
	vortex_thread_pool.c \
	vortex_channel_pool.c \
	vortex_win32.c \
	vortex_errno.c \
	vortex_thread.c 

libvortex_1_1_include_HEADERS = vortex.h \
	vortex_ctx.h \
	vortex_ctx_private.h \
	vortex_support.h \
	vortex_connection.h \
	vortex_listener.h \
	vortex_handlers.h \
	vortex_types.h \
	vortex_frame_factory.h \
	vortex_greetings.h \
	vortex_profiles.h \
	vortex_channel.h \
	vortex_io.h \
	vortex_reader.h \
	vortex_dtds.h \
	vortex_hash.h \
	vortex_queue.h \
	vortex_sequencer.h \
	vortex_thread_pool.h \
	vortex_channel_pool.h \
	vortex_win32.h \
	vortex_errno.h \
	vortex_thread.h \
	vortex-channel.dtd.h \
	vortex-listener-conf.dtd.h 

libvortex_1_1_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)

libvortex_1_1_la_LDFLAGS = -no-undefined -export-symbols-regex '^(vortex|__vortex|_vortex).*'

libvortex-1.1.def: update-def

update-dtds:
	axl-knife --input ../data/channel.dtd --dtd-to-c --output vortex-channel.dtd.h --ifnewer
	axl-knife --input ../data/vortex-listener-conf.dtd --dtd-to-c  --output vortex-listener-conf.dtd.h  --ifnewer

update-def:
	echo "EXPORTS" > libvortex-1.1.def
	cat .libs/libvortex-1.1.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __vortex >> libvortex-1.1.def