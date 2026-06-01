TARGET = mcp_bridge
TEMPLATE = lib
CONFIG += plugin

QT += widgets network

INCLUDEPATH += ../../librecad/src/plugins \
               ../../librecad/src/main \
               ../../librecad/src/ui/main \
               ../../librecad/src/lib/engine \
               ../../librecad/src/lib/actions \
               ../../librecad/src/lib/gui \
               ../../librecad/src/ui \
               ../../librecad/src/ui/view \
               ../../librecad/src/ui/dock_widgets

HEADERS += mcp_bridge.h \
           mcp_domain.h \
           librecad_adapter.h \
           command_processor.h

SOURCES += mcp_bridge.cpp \
           librecad_adapter.cpp \
           command_processor.cpp

DESTDIR = ../../generated/plugins
