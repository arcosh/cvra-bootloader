#
# This file is included at the beginning
# of all platform Makefiles.
# It evaluates make console arguments and sets
# the Makefile/compiler verbosity level accordingly.
#
# This file mainly exports the symbols PRINT and Q.
#

# With COLOR and NO_COLOR the console output can be colored.
ifdef COLOR
ifeq ($(COLOR),0)
NO_COLOR = 1
endif
ifeq ($(COLOR),no)
NO_COLOR = 1
endif
ifeq ($(COLOR),off)
NO_COLOR = 1
endif
endif

ifndef NO_COLOR
ANSI_RED = \033[1;31m
ANSI_RESET = \033[0m
PRINT = @printf "$(ANSI_RED)%s$(ANSI_RESET)\n"
else
PRINT = @printf "%s\n"
endif

# With VERBOSE the QUIET option (below) is being disabled.
ifdef VERBOSE
ifeq ($(VERBOSE),1)
QUIET = 0
endif
ifeq ($(VERBOSE),yes)
QUIET = 0
endif
ifeq ($(VERBOSE),on)
QUIET = 0
endif
endif

# With QUIET the invoked commands are not shown in the output.
ifndef QUIET
QUIET = 1
endif

Q :=
ifeq ($(QUIET),1)
Q := @
endif
ifeq ($(QUIET),yes)
Q := @
endif
ifeq ($(QUIET),on)
Q := @
endif
