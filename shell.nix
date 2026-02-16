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
    # 设置库路径，确保 SDL3 可以动态加载 X11/Wayland 库
    export LD_LIBRARY_PATH=${pkgs.libGL}/lib:${pkgs.mesa}/lib:${pkgs.xorg.libX11}/lib:${pkgs.xorg.libXext}/lib:${pkgs.xorg.libXcursor}/lib:${pkgs.xorg.libXrandr}/lib:${pkgs.xorg.libXi}/lib:${pkgs.xorg.libXfixes}/lib:${pkgs.libffi}/lib:${pkgs.wayland}/lib:${pkgs.libxkbcommon}/lib:${pkgs.pipewire}/lib:${pkgs.vulkan-loader}/lib:$LD_LIBRARY_PATH

    # 设置 PKG_CONFIG_PATH
    export PKG_CONFIG_PATH=${pkgs.wayland}/lib/pkgconfig:${pkgs.libxkbcommon}/lib/pkgconfig:${pkgs.libffi}/lib/pkgconfig:$PKG_CONFIG_PATH

    # Vulkan ICD 路径 - 指向 Mesa 驱动（包含 lavapipe 软件渲染）
    export VK_ICD_FILENAMES=${pkgs.mesa.drivers}/share/vulkan/icd.d/lvp_icd.x86_64.json:${pkgs.mesa.drivers}/share/vulkan/icd.d/intel_icd.x86_64.json:${pkgs.mesa.drivers}/share/vulkan/icd.d/radeon_icd.x86_64.json

    # 自动检测并使用合适的后端
    if [ -n "$WAYLAND_DISPLAY" ]; then
      export SDL_VIDEODRIVER=wayland
    elif [ -n "$DISPLAY" ]; then
      export SDL_VIDEODRIVER=x11
    fi

    echo "Nix 开发环境已加载"
    echo "使用 $SDL_VIDEODRIVER 后端"
    echo "Vulkan ICD: $VK_ICD_FILENAMES"
  '';
}
