{
  "targets": [
    {
      "target_name": "phantom_vault_addon",
      "sources": [
        "src/addon.cpp",
        "src/vault_wrapper.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../core/include",
        "../../core/build/include",
        "/usr/include/qt5",
        "/usr/include/x86_64-linux-gnu/qt5",
        "/usr/include/openssl"
      ],
      "libraries": [
        "-L<(module_root_dir)/../../core/build",
        "-lphantom_vault_core",
        "-Wl,-rpath,<(module_root_dir)/../../core/build"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "cflags_cc": [ "-std=c++20" ],
      "defines": [ 
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "NODE_ADDON_API_ENABLE_MAYBE"
      ],
      "conditions": [
        ["OS=='linux'", {
          "libraries": [
            "-Wl,-rpath,$ORIGIN/../../core/build"
          ]
        }]
      ]
    }
  ]
}