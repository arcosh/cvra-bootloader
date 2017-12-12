#
# make targets for flashing with openocd
#

.PHONY: flash
flash: all
	openocd -f oocd.cfg -c "init" -c "reset halt" -c "program $(PROJNAME).elf verify reset exit"

.PHONY: reset
reset:
	openocd -f oocd.cfg -c "init" -c "reset run" -c "shutdown"
