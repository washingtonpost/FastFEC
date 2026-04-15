const std = @import("std");
const builtin = @import("builtin");

pub fn linkPcre(vendored_pcre: bool, libExe: *std.Build.Step.Compile) void {
    if (vendored_pcre) {
        libExe.addCSourceFiles(.{ .files = &pcreSources, .flags = &buildOptions });
        // When vendoring PCRE, define PCRE_STATIC to avoid dllimport on Windows
        libExe.root_module.addCMacro("PCRE_STATIC", "1");
    } else {
        if (builtin.os.tag == .windows) {
            libExe.linkSystemLibrary("pcre");
        } else {
            libExe.linkSystemLibrary("libpcre");
        }
    }
    if (libExe.rootModuleTarget().os.tag.isDarwin()) {
        // useful for package maintainers
        // see https://github.com/ziglang/zig/issues/13388
        libExe.headerpad_max_install_names = true;
    }
}

fn createModule(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Module {
    return b.createModule(.{
        .target = target,
        .optimize = optimize,
    });
}

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .ReleaseFast,
    });

    const lib_only: bool = b.option(bool, "lib-only", "Only compile the library") orelse false;
    const skip_lib: bool = b.option(bool, "skip-lib", "Skip compiling the library") orelse false;
    const wasm: bool = b.option(bool, "wasm", "Compile the wasm library") orelse false;
    const vendored_pcre: bool = b.option(bool, "vendored-pcre", "Use vendored pcre") orelse true;

    // Main build step
    if (!lib_only and !wasm) {
        const fastfec_cli = b.addExecutable(.{
            .name = "fastfec",
            .root_module = createModule(b, target, optimize),
        });

        fastfec_cli.linkLibC();

        fastfec_cli.addCSourceFiles(.{ .files = &libSources, .flags = &buildOptions });
        linkPcre(vendored_pcre, fastfec_cli);
        fastfec_cli.addCSourceFiles(.{
            .files = &.{
                "src/cli.c",
                "src/main.c",
            },
            .flags = &buildOptions,
        });
        b.installArtifact(fastfec_cli);
    }

    if (!wasm and !skip_lib) {
        // Library build step
        const fastfec_lib = b.addLibrary(.{
            .name = "fastfec",
            .linkage = .dynamic,
            .root_module = createModule(b, target, optimize),
        });
        if (fastfec_lib.rootModuleTarget().os.tag.isDarwin()) {
            // useful for package maintainers
            // see https://github.com/ziglang/zig/issues/13388
            fastfec_lib.headerpad_max_install_names = true;
        }
        fastfec_lib.linkLibC();
        fastfec_lib.addCSourceFiles(.{ .files = &libSources, .flags = &buildOptions });
        linkPcre(vendored_pcre, fastfec_lib);
        b.installArtifact(fastfec_lib);
    } else if (wasm) {
        // Wasm library build step
        const wasm_target: std.Target.Query = .{ .cpu_arch = .wasm32, .os_tag = .wasi };
        const wasm_module = b.createModule(.{
            .target = b.resolveTargetQuery(wasm_target),
            .optimize = optimize,
            .link_libc = true,
        });
        const fastfec_wasm = b.addExecutable(.{
            .name = "fastfec",
            .root_module = wasm_module,
        });
        fastfec_wasm.rdynamic = true;
        fastfec_wasm.entry = .disabled;
        fastfec_wasm.import_symbols = true;
        fastfec_wasm.addCSourceFiles(.{ .files = &libSources, .flags = &buildOptions });
        linkPcre(vendored_pcre, fastfec_wasm);
        fastfec_wasm.addCSourceFile(.{ .file = .{
            .cwd_relative = "src/wasm.c",
        }, .flags = &buildOptions });
        b.installArtifact(fastfec_wasm);
    }

    // Test step
    var prev_test_step: ?*std.Build.Step = null;
    for (tests) |test_file| {
        const base_file = std.fs.path.basename(test_file);
        const subtest_exe = b.addExecutable(.{
            .name = base_file,
            .root_module = createModule(b, target, optimize),
        });
        subtest_exe.linkLibC();
        subtest_exe.addCSourceFiles(.{ .files = &testIncludes, .flags = &buildOptions });
        linkPcre(vendored_pcre, subtest_exe);
        subtest_exe.addCSourceFile(.{
            .file = .{ .cwd_relative = test_file },
            .flags = &buildOptions,
        });
        const subtest_cmd = b.addRunArtifact(subtest_exe);
        if (prev_test_step != null) {
            subtest_cmd.step.dependOn(prev_test_step.?);
        }
        prev_test_step = &subtest_cmd.step;
    }
    const test_steps = prev_test_step.?;
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(test_steps);
}

const libSources = [_][]const u8{
    "src/buffer.c",
    "src/memory.c",
    "src/encoding.c",
    "src/csv.c",
    "src/writer.c",
    "src/fec.c",
};
const pcreSources = [_][]const u8{
    "src/pcre/pcre_chartables.c",
    "src/pcre/pcre_byte_order.c",
    "src/pcre/pcre_compile.c",
    "src/pcre/pcre_config.c",
    "src/pcre/pcre_dfa_exec.c",
    "src/pcre/pcre_exec.c",
    "src/pcre/pcre_fullinfo.c",
    "src/pcre/pcre_get.c",
    "src/pcre/pcre_globals.c",
    "src/pcre/pcre_jit_compile.c",
    "src/pcre/pcre_maketables.c",
    "src/pcre/pcre_newline.c",
    "src/pcre/pcre_ord2utf8.c",
    "src/pcre/pcre_refcount.c",
    "src/pcre/pcre_string_utils.c",
    "src/pcre/pcre_study.c",
    "src/pcre/pcre_tables.c",
    "src/pcre/pcre_ucd.c",
    "src/pcre/pcre_valid_utf8.c",
    "src/pcre/pcre_version.c",
    "src/pcre/pcre_xclass.c",
};
const tests = [_][]const u8{ "src/buffer_test.c", "src/csv_test.c", "src/writer_test.c", "src/cli_test.c" };
const testIncludes = [_][]const u8{ "src/buffer.c", "src/memory.c", "src/encoding.c", "src/csv.c", "src/writer.c", "src/cli.c" };
const buildOptions = [_][]const u8{
    "-std=c11",
    "-pedantic",
    "-std=gnu99",
    "-Wall",
    "-W",
    "-Wno-missing-field-initializers",
};
