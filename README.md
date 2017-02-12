# IncludeOS + lua

This runs lua repl on qemu virtual machine.

## Install IncludeOS

```
git clone https://github.com/hioa-cs/IncludeOS.git
cd IncludeOS
mkdir build
cd build
cmake ..
make -j5
sudo make install
```

## Build and run this image

```
mkdir build
cd build
cmake ..
make
../run.sh IncluaOS
```

Plus, `telnet 10.0.0.42` and type `lua`

## Lua version

Lua 5.3.4, released on 12 Jan 2017.
