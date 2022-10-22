load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/usr/bin/clang",
        ),
        tool_path(
            name = "ld",
            path = "/usr/bin/ld",
        ),
        tool_path(
            name = "ar",
            path = "/usr/bin/ar",
        ),
        tool_path(
            name = "cpp",
            path = "/usr/bin/cpp",
        ),
        tool_path(
            name = "gcov",
            path = "/bin/false",
        ),
        tool_path(
            name = "nm",
            path = "/usr/false",
        ),
        tool_path(
            name = "objdump",
            path = "/bin/false",
        ),
        tool_path(
            name = "strip",
            path = "/bin/false",
        ),
    ]

    homebrew_libs = [
        "folly/2022.10.17.00/lib/libfolly.dylib",
        "fmt/9.1.0/lib/libfmt.dylib",
        "glog/0.6.0/lib/libglog.dylib",
        "gflags/2.2.2/lib/libgflags.dylib",
        "protobuf/21.8/lib/libprotobuf.dylib",
        "grpc/1.50.0/lib/libgrpc++.1.50.0.dylib",
        "grpc/1.50.0/lib/libgrpc++_reflection.1.50.0.dylib",
        "grpc/1.50.0/lib/libgpr.28.0.0.dylib",
        "v8/10.6.194.18/lib/libv8.dylib",
        "v8/10.6.194.18/lib/libv8_libbase.dylib",
        "v8/10.6.194.18/lib/libv8_libplatform.dylib",
    ]

    dylibs = ["/usr/local/Cellar/{}".format(lib) for lib in homebrew_libs]

    features = [
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = ["-lstdc++"] + dylibs,
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "archiver_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = [
                        "c++-link-static-library",
                        "c++-link-alwayslink-static-library",
                        "c++-link-pic-static-library",
                        "c++-link-alwayslink-pic-static-library",
                    ],
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "r",
                                "%{output_execpath}",
                            ],
                        ),
                        flag_group(
                            iterate_over = "libraries_to_link",
                            flags = ["%{libraries_to_link.name}"],
                        ),
                    ]),
                ),
            ],
        ),
    ]
    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = [
            "/Library/Developer/CommandLineTools/usr/lib/clang/13.1.6/include",
            "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include",
            "/usr/local/include/",
        ],
        toolchain_identifier = "local",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "k8",
        target_libc = "unknown",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)
