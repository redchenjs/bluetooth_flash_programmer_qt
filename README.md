Easy Flash Programmer
=====================

User tools for Bluetooth Easy Flash Programmer.

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
easy-flash-programmer /dev/rfcommX erase_all
```

### Erase flash

```
easy-flash-programmer /dev/rfcommX erase 0x0000(addr) 0x1000(length)
```

### Write flash

```
easy-flash-programmer /dev/rfcommX write 0x0000(addr) 0x1000(length) data.bin
```

### Read flash

```
easy-flash-programmer /dev/rfcommX read 0x0000(addr) 0x1000(length) data.bin
```

### Read flash info

```
easy-flash-programmer /dev/rfcommX info
```
