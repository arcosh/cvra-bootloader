#!bash

#
# This script uses PyInstaller
# to generate standalone Windows executables
# for the bootloader client Python scripts.
#
# Requirements: git-bash, Python 3.5, PyInstaller
#

# Clone Python 3.x-compatible progressbar
if [ ! -e progressbar ]; then
	git clone https://github.com/niltonvolpato/python-progressbar progressbar
	mv -v progressbar/progressbar/* progressbar/
fi

# Install dependencies
pip3 install pyinstaller
pip3 install pyserial
pip3 install msgpack-python

for script in bootloader_flash read_config write_config change_id run_application; do
	rm build/$script -fr
	rm dist/$script.exe
	pyinstaller --onefile cvra_bootloader/$script.py
	rm $script.spec
	rm build/$script -fr
done

for exe in read_config write_config change_id run_application; do
	mv -v dist/$exe.exe dist/bootloader_$exe.exe
done

# Remove (empty) build folder
rmdir build

