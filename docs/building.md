# Building n64tools
This document contains instructions for building the n64tools collection from source.

## Dependencies
n64tools requires the following dependencies to be installed:

## MacOS
- [Homebrew](https://brew.sh/)
```bash
brew install libpng capstone libyaml
```

## Linux
- [apt](https://packages.ubuntu.com/)
```bash
sudo apt install libpng-dev libcapstone-dev libyaml-dev
```

## Windows
<= COMING SOON =>

## Building
Building is a simple process, and can be done with the following command:
```bash
uv run scripts/build.py --help
```
