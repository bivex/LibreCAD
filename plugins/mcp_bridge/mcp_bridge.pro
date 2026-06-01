QT       += widgets network
TEMPLATE = lib
CONFIG += plugin
VERSION = 1.0.0
TARGET = $$qtLibraryTarget(mcp_bridge)

GENERATED_DIR = ../../generated/plugin/mcp_bridge
# Use common project definitions.
include(../../common.pri)

# For plugins
INCLUDEPATH    += ../../librecad/src/plugins \
                  ../../librecad/src/main \
                  ../../librecad/src/main/intern \
                  ../../librecad/src/ui/main \
                  ../../librecad/src/ui/view \
                  ../../librecad/src/ui/dock_widgets \
                  ../../librecad/src/lib/actions \
                  ../../librecad/src/lib/gui \
                  ../../librecad/src/lib/engine \
                  ../../librecad/src/lib/engine/document \
                  ../../librecad/src/lib/engine/document/entities \
                  ../../librecad/src/lib/engine/document/container \
                  ../../librecad/src/lib/engine/document/layers \
                  ../../librecad/src/lib/engine/document/blocks \
                  ../../librecad/src/lib/engine/document/variables \
                  ../../librecad/src/lib/engine/document/dimstyles \
                  ../../librecad/src/lib/engine/document/textstyles \
                  ../../librecad/src/lib/engine/document/ucs \
                  ../../librecad/src/lib/engine/document/views \
                  ../../librecad/src/lib/engine/undo \
                  ../../librecad/src/lib/math \
                  ../../librecad/src/lib/debug \
                  ../../libraries/libdxfrw/src \
                  ../../libraries/jwwlib/src

HEADERS += mcp_bridge.h \
           mcp_domain.h \
           librecad_adapter.h \
           command_processor.h

SOURCES += mcp_bridge.cpp \
           librecad_adapter.cpp \
           command_processor.cpp

macx {
    DESTDIR = ../../LibreCAD.app/Contents/Resources/plugins
} else {
    DESTDIR = ../../generated/plugins
}
