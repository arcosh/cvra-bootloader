#!bash

#
# This script generates standalone bootloader executables for Windows
#
# Requirements: git, bash, Python 3.5
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

# Remove build folder
rmdir build

