# LVGL test project on Linux framebuffer (RPI 4B)

This is my fork of [LVGL Linux example](https://github.com/lvgl/lv_port_linux)

## Clone the project

Clone the project

```sh
git clone https://github.com/oktilon/lvgl_test_rpi.git
cd lvgl_test_rpi/
```

LVGL is a submodule of `lvgl_test_rpi`, use the following command
to fetch it, it will be downloaded to the `lvgl/` directory

```sh
git submodule update --init --recursive
```

## Build

```sh
cmake -B build
cmake --build build -j$(nproc)
```

## Run the application

```
./build/bin/lvgl_test
```

## Permissions

By default, unprivileged users don't have access to the framebuffer device `/dev/fb0`. In such cases, you can either run the application
with `sudo` privileges or you can grant access to the `video` group.

```bash
sudo adduser $USER video
newgrp video
./build/bin/lvgl_test
```