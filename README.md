## Build project
```
mkdir -p build
cd build
cmake .. -G Ninja
ninja
```

## Flash to ESP32
```
ninja flash
```