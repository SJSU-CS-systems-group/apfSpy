# DMG Info Tool

Reads in a DMG file and outputs structure information.

## Dependencies

- libXML2

## Get Started

First, install the dependencies.
On Linux, run the command:

```sh
sudo apt install libxml2-dev
sudo apt install zlib1g-dev
```

To open the project in an IDE, follow the libXML2 installation instructions for your IDE.
Installation instructions for VisualStudio can be found [here](https://www.youtube.com/watch?v=qZFtFIYQRGs).

## Run Program

On Linux run the Makefile to build the project. Then execute the DMG binary program.
```sh
make clean
make
./APFSpy <DMG_FILE> {Options}

#Usage:
./APFSpy <DMG_FILE>                             Prints the Disk Image Structure
                        -c                      Container Superblock Information
                        -v [ Volume ID ]        All Vol SuperBlock Information | Specified Volume's Information
                        -f <file_name>          Displays file content
                        -v <Volume_ID> -fs      Displays File system Structure
                        -d                      Debug Mode
```
