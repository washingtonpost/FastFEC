const std = @import("std");
const builtin = @import("builtin");
const CrossTarget = std.zig.CrossTarget;

pub fn build(b: *std.build.Builder) !void {
    b.setPreferredReleaseMode(.ReleaseFast);
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const lib_only: bool = b.option(bool, "lib-only", "Only compile the library") orelse false;
    const skip_lib: bool = b.option(bool, "skip-lib", "Skip compiling the library") orelse false;
    const wasm: bool = b.option(bool, "wasm", "Compile the wasm library") orelse false;

    // Main build step
    if (!lib_only and !wasm) {
        const fastfec_cli = b.addExecutable("fastfec", null);
        fastfec_cli.setTarget(target);
        fastfec_cli.setBuildMode(mode);
        fastfec_cli.install();

        fastfec_cli.linkLibC();

        fastfec_cli.addCSourceFiles(&libSources, &buildOptions);
        fastfec_cli.addCSourceFiles(&pcreSources, &buildOptions);
        fastfec_cli.addCSourceFiles(&.{
            "src/cli.c",
            "src/main.c",
        }, &buildOptions);
    }

    if (!wasm and !skip_lib) {
        // Library build step
        const fastfec_lib = b.addSharedLibrary("fastfec", null, .unversioned);
        fastfec_lib.setTarget(target);
        fastfec_lib.setBuildMode(mode);
        fastfec_lib.install();
        fastfec_lib.linkLibC();
        fastfec_lib.addCSourceFiles(&libSources, &buildOptions);
        fastfec_lib.addCSourceFiles(&pcreSources, &buildOptions);
    } else if (wasm) {
        // Wasm library build step
        const fastfec_wasm = b.addSharedLibrary("fastfec", null, .unversioned);
        const wasm_target = CrossTarget{ .cpu_arch = .wasm32, .os_tag = .wasi };
        fastfec_wasm.setTarget(wasm_target);
        fastfec_wasm.setBuildMode(mode);
        fastfec_wasm.install();
        fastfec_wasm.linkLibC();
        fastfec_wasm.addCSourceFiles(&libSources, &buildOptions);
        fastfec_wasm.addCSourceFiles(&pcreSources, &buildOptions);
        fastfec_wasm.addCSourceFile("src/wasm.c", &buildOptions);
    }

    // Test step
    var prev_test_step: ?*std.build.Step = null;
    for (tests) |test_file| {
        const base_file = std.fs.path.basename(test_file);
        const subtest_exe = b.addExecutable(base_file, null);
        subtest_exe.linkLibC();
        subtest_exe.addCSourceFiles(&testIncludes, &buildOptions);
        subtest_exe.addCSourceFiles(&pcreSources, &buildOptions);
        subtest_exe.addCSourceFile(test_file, &buildOptions);
        const subtest_cmd = subtest_exe.run();
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
const testIncludes = [_][]const u8{
    "src/buffer.c",
    "src/memory.c",
    "src/encoding.c",
    "src/csv.c",
    "src/writer.c",
    "src/cli.c"
};
const buildOptions = [_][]const u8{
    "-std=c11",
    "-pedantic",
    "-Wall",
    "-W",
    "-Wno-missing-field-initializers",
};
