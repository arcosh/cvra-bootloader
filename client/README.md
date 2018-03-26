# Bootloader client

This repository contains the source code for the bootloader's Python client.
The client communicates with the bootloader on a target device via CAN.
Among other things it facilitates the firmware image upload
into the target device's flash memory.

# Installation

The client requires Python 3 (Python 3.5 is used for development).
Also, there are a couple of dependencies,
which can be installed automatically
either via your system's package manager
or using pip.
The latter method is demonstrated below.

## virtualenv

In order to avoid cluttering your system's Python installation
while installing dependencies using pip,
you should not install them system wide,
but into a separate runtime environment.

virtualenv will be used here to create such an environment.
In case, you don't already have virtualenv,
you can install it using pip:
```sh
# Installing virtualenv
pip install virtualenv
```

## Linux
```sh
# Creating the virtual environment
virtualenv --python=python3 venv

# Activating the virtual environment
source venv/bin/activate

# Activating the virtual environment, if you use fish
source venv/bin/activate.fish

# Finally, install everything into your virtual environment
python setup.py install
```

## Windows
```sh
# Creating the virtual environment with the full path to Python 3.x executable
virtualenv --python="C:/Program Files/Python35/python.exe" venv

# Activating the virtual environment
source venv/Scripts/activate

# Finally, install everything into your virtual environment
python setup.py install
```

## Windows standalone executables

It is possible to create standalone executables (.exe files)
for each of the bootloader client's tools (see tool list below)
using PyInstaller.

In order to do so,
install the git-bash,
start it
and run the `build-windows-standalone.sh` script
from the client folder.
The script attempts to clone and download
a couple of dependencies
and proceeds by packing EXEs for all tools.

After the script completes,
the EXE files can be found in the `dist` subfolder.

Note:
This procedure only tested successfully
with Python 3.5 in combination with PyInstaller 3.3.

## Development mode

If you are working on the bootloader client's code,
you might want to use `python setup.py develop`.
This symlinks the Python code into your runtime environment instead of copying it,
meaning you can edit the code and run the modified version
without having to run `setup.py` again.

# Tests

To run the tests, execute `python -m unittest discover`
after installing all Python dependencies.

# Built-in tools

These are the tools for communicating with the bootloader on a target device.
All tools have an argument `-h/--help`,
so use that, to know which arguments you must provide to them.

* `bootloader_flash`: Used to upload new firmware onto target boards.
* `bootloader_invoke`: Used to ping a target device, until it responds.
* `bootloader_read_config`: Used to read the config from a bunch of boards and dump it as JSON.
* `bootloader_write_config`: Used to change board config, such as device class, name and so on.
* `bootloader_change_id`: Used to change a single device's ID (*Use carefully*).
