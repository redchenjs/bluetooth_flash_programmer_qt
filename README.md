Bluetooth Flash Programmer
==========================

Bluetooth Flash Programmer using Bluetooth SPP profile.

## Dependencies

```
cmake
qt5-base
qt5-connectivity
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

### Get flash info

```
btflash BD_ADDR info
```
