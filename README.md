SPP Flash Programmer
====================

## Dependencies

```
bluez
cmake
pkg-config
qt5-default
qt5serialport-dev
```

## Build

```
mkdir -p build
cd build
cmake ../
make
```

## Usage

```
sudo rfcomm bind hciX XX:XX:XX:XX:XX:XX
```

### Erase full flash chip

```
spp-flash-programmer /dev/rfcommX erase_all
```

### Erase flash

```
spp-flash-programmer /dev/rfcommX erase 0x0000(addr) 0x1000(length)
```

### Write flash

```
spp-flash-programmer /dev/rfcommX write 0x0000(addr) 0x1000(length) data.bin
```

### Read flash

```
spp-flash-programmer /dev/rfcommX read 0x0000(addr) 0x1000(length) data.bin
```

### Read flash info

```
spp-flash-programmer /dev/rfcommX info
```
