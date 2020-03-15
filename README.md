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
bluetooth-flash-programmer BD_ADDR erase_all
```

### Erase flash

```
bluetooth-flash-programmer BD_ADDR erase 0x0000(addr) 0x1000(length)
```

### Write flash

```
bluetooth-flash-programmer BD_ADDR write 0x0000(addr) 0x1000(length) data.bin
```

### Read flash

```
bluetooth-flash-programmer BD_ADDR read 0x0000(addr) 0x1000(length) data.bin
```

### Read flash info

```
bluetooth-flash-programmer BD_ADDR info
```
