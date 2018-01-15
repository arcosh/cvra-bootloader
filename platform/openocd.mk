#
# make targets for flashing with openocd
#

.PHONY: openocd flash reset

openocd: all
	openocd -f oocd.cfg -c "init" -c "reset halt"

flash: all
	openocd -f oocd.cfg -c "init" -c "reset halt" -c "program $(PROJNAME).elf verify reset exit"

reset:
	openocd -f oocd.cfg -c "init" -c "reset run" -c "shutdown"
