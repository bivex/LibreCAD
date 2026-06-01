QT       += widgets network
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.0
TARGET = $$qtLibraryTarget(mcp_bridge)

GENERATED_DIR = ../../generated/plugin/mcp_bridge
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins

HEADERS += mcp_bridge.h \
           mcp_domain.h \
           librecad_adapter.h \
           command_processor.h

SOURCES += mcp_bridge.cpp \
           librecad_adapter.cpp \
           command_processor.cpp

DESTDIR = ../../generated/plugins
