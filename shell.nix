{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    cmake
    ninja
    pkg-config
    boost190
    sdl3
    sqlite  # SQLite3 开发包
    # OpenGL/Vulkan
    mesa
    libGL
    vulkan-loader
    vulkan-headers
    vulkan-tools
    mesa.drivers # 包含 Intel/AMD/Software Vulkan 驱动
    egl-wayland
    # Wayland
    wayland
    wayland-protocols
    libxkbcommon
    wayland-scanner
    libffi
    # X11
    xorgproto
    libX11
    libXext
    libXcursor
    libXrandr
    libXi
    libXfixes
    libXScrnSaver
    libXtst
    libxcb
    xcbutil
    xcbutilcursor
    xcbutilkeysyms
    xcbutilwm
    # PipeWire (optional)
    pipewire
  ];
  shellHook = ''
    # ========== 头文件路径 ==========
    export CPATH=${pkgs.boost190.dev}/include:${pkgs.xorgproto}/include:${pkgs.libX11.dev}/include:${pkgs.libXext.dev}/include:${pkgs.libXcursor.dev}/include:${pkgs.libXrandr.dev}/include:${pkgs.libXi.dev}/include:${pkgs.libXfixes.dev}/include:${pkgs.libxcb.dev}/include:${pkgs.wayland.dev}/include:${pkgs.libxkbcommon.dev}/include:${pkgs.libffi.dev}/include:${pkgs.libGL.dev}/include:${pkgs.vulkan-headers}/include:$CPATH

    # ========== CMake 前缀路径 ==========
    export CMAKE_PREFIX_PATH=${pkgs.boost190}:${pkgs.boost190.dev}:${pkgs.libX11.dev}:${pkgs.wayland.dev}:${pkgs.libGL.dev}:${pkgs.sdl3.dev}:${pkgs.sqlite.dev}:$CMAKE_PREFIX_PATH

    # ========== Boost 路径 ==========
    export BOOST_ROOT=${pkgs.boost190}

    # ========== 库路径，确保 SDL3 可以动态加载 X11/Wayland 库 ==========
    export LD_LIBRARY_PATH=${pkgs.boost190}/lib:${pkgs.libGL}/lib:${pkgs.mesa}/lib:${pkgs.libX11}/lib:${pkgs.libXext}/lib:${pkgs.libXcursor}/lib:${pkgs.libXrandr}/lib:${pkgs.libXi}/lib:${pkgs.libXfixes}/lib:${pkgs.libffi}/lib:${pkgs.wayland}/lib:${pkgs.libxkbcommon}/lib:${pkgs.pipewire}/lib:${pkgs.vulkan-loader}/lib:$LD_LIBRARY_PATH

    # ========== PKG_CONFIG_PATH ==========
    export PKG_CONFIG_PATH=${pkgs.boost190.dev}/lib/pkgconfig:${pkgs.wayland}/lib/pkgconfig:${pkgs.libxkbcommon}/lib/pkgconfig:${pkgs.libffi}/lib/pkgconfig:${pkgs.libX11.dev}/lib/pkgconfig:${pkgs.libGL.dev}/lib/pkgconfig:${pkgs.sqlite.dev}/lib/pkgconfig:$PKG_CONFIG_PATH

    # ========== PKG_CONFIG_BIN ==========
    export PATH=${pkgs.pkg-config}/bin:$PATH

    # ========== Vulkan ICD 路径 - 指向 Mesa 驱动（包含 lavapipe 软件渲染） ==========
    export VK_ICD_FILENAMES=${pkgs.mesa.drivers}/share/vulkan/icd.d/lvp_icd.x86_64.json:${pkgs.mesa.drivers}/share/vulkan/icd.d/intel_icd.x86_64.json:${pkgs.mesa.drivers}/share/vulkan/icd.d/radeon_icd.x86_64.json

    # ========== 自动检测并使用合适的后端 ==========
    if [ -n "$WAYLAND_DISPLAY" ]; then
      export SDL_VIDEODRIVER=wayland
    elif [ -n "$DISPLAY" ]; then
      export SDL_VIDEODRIVER=x11
    fi

    echo "Nix 开发环境已加载"
    echo "使用 $SDL_VIDEODRIVER 后端"
    echo "Boost: ${pkgs.boost190}"
    echo "Vulkan ICD: $VK_ICD_FILENAMES"
  '';
}
