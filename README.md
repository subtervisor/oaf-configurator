# OAF Configurator

A simple configuration UI for [open_agb_firm](https://github.com/profi200/open_agb_firm).

## Screenshot

<img width="400" height="480" alt="Screenshot of OAF Configurator" src="https://github.com/user-attachments/assets/13dbf5fc-9347-4ebe-9f89-7a6af39a7166" /> <img width="400" height="480" alt="Screenshot of OAF Configurator" src="https://github.com/user-attachments/assets/57083673-56c0-4aa9-bb94-5b63a5db1564" />

## Building

```
git submodule update --init --recursive
cmake -S. -Bbuild -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

# Credits
- [NPI-D7](https://github.com/npid7) : created imgui-impl-ctr
- [Tobi-D7](https://github.com/Tobi-D7) : make the code more imgui_impl like
- [mtheall](https://github.com/mtheall) : ftpd's imgui code
- [devkitpro](https://github.com/devkitPro) : libctru, citro3d 
- [NPI-D7](https://github.com/npid7) : imgui-impl-ctr
