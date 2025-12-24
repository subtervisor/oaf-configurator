# OAF Configurator

A simple configuration UI for [open_agb_firm](https://github.com/profi200/open_agb_firm).

## Building

```
git submodule update --init --recursive
cmake -S. -Bbuild -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" -DCMAKE_BUILD_TYPE=Release -DIMGUI_CTR_BUILD_EXAMPLE=ON
cmake --build build
```

# Credits
- [NPI-D7](https://github.com/npid7) : created imgui-impl-ctr
- [Tobi-D7](https://github.com/Tobi-D7) : make the code more imgui_impl like
- [mtheall](https://github.com/mtheall) : ftpd's imgui code
- [devkitpro](https://github.com/devkitPro) : libctru, citro3d 
- [NPI-D7](https://github.com/npid7) : imgui-impl-ctr
