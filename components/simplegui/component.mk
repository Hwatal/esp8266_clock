COMPONENT_ADD_INCLUDEDIRS := GUI/inc HMI/inc port DemoProc/inc
COMPONENT_SRCDIRS := .
COMPONENT_SRCDIRS += GUI/src HMI/src port DemoProc/src

CFLAGS += -Wno-error=maybe-uninitialized

