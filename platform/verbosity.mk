#
# This file is included at the beginning
# of all platform Makefiles.
# It evaluates make console arguments and sets
# the Makefile/compiler verbosity level accordingly.
#
# This file mainly exports the symbols PRINT and Q.
#

ANSI_RED = \033[1;31m
ANSI_RESET = \033[0m
PRINT = @printf "$(ANSI_RED)%s$(ANSI_RESET)\n"

# e.g. make VERBOSE=1 generates additional output
ifdef VERBOSE
QUIET = 0
endif

# e.g. make QUIET=0 generates additional output
ifndef QUIET
QUIET = 1
endif

# If QUIET != 1, then the invoked commands are shown in the output.
ifeq ($(QUIET),1)
Q = @
endif
