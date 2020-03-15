Bluetooth Flash Programmer
==========================

User tools for Bluetooth Flash Programmer.

## Dependencies

```
cmake
pkg-config
qt5-default
qt5connectivity-dev
```

## Build

```
mkdir -p build
cd build
cmake ../
make
```

## Usage

### Erase full flash chip

```
btflash BD_ADDR erase_all
```

### Erase flash

```
btflash BD_ADDR erase 0x0000(addr) 0x1000(length)
```

### Write flash

```
btflash BD_ADDR write 0x0000(addr) 0x1000(length) data.bin
```

### Read flash

```
btflash BD_ADDR read 0x0000(addr) 0x1000(length) data.bin
```

### Read flash info

```
btflash BD_ADDR info
```
