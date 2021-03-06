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
    const vendored_pcre: bool = b.option(bool, "vendored-pcre", "Use vendored pcre (for non-Windows platforms)") orelse true;

    // Compile pcre
    const pcre = b.addStaticLibrary("pcre", null);
    pcre.setTarget(target);
    pcre.linkLibC();
    pcre.addIncludeDir(pcreIncludeDir);
    pcre.addCSourceFiles(&pcreSources, &buildOptions);

    // Main build step
    if (!lib_only and !wasm) {
        const fastfec_cli = b.addExecutable("fastfec", null);
        fastfec_cli.setTarget(target);
        fastfec_cli.setBuildMode(mode);
        fastfec_cli.install();

        // Add curl
        fastfec_cli.linkLibC();
        if (builtin.os.tag == .windows) {
            fastfec_cli.linkSystemLibrary("ws2_32");
            fastfec_cli.linkSystemLibrary("advapi32");
            fastfec_cli.linkSystemLibrary("crypt32");
            fastfec_cli.linkSystemLibrary("libcurl");
            fastfec_cli.linkSystemLibraryName("zlib");
            fastfec_cli.linkSystemLibrary("pcre");
        } else {
            fastfec_cli.linkSystemLibrary("curl");
            if (vendored_pcre) {
                fastfec_cli.addIncludeDir(pcreIncludeDir);
                fastfec_cli.linkLibrary(pcre);
            } else {
                fastfec_cli.linkSystemLibrary("libpcre");
            }
        }

        fastfec_cli.addCSourceFiles(&libSources, &buildOptions);
        fastfec_cli.addCSourceFiles(&.{
            "src/urlopen.c",
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
        if (builtin.os.tag == .windows) {
            fastfec_lib.linkSystemLibrary("pcre");
        } else {
            if (vendored_pcre) {
                fastfec_lib.addIncludeDir(pcreIncludeDir);
                fastfec_lib.linkLibrary(pcre);
            } else {
                fastfec_lib.linkSystemLibrary("libpcre");
            }
        }
        fastfec_lib.addCSourceFiles(&libSources, &buildOptions);
    } else if (wasm) {
        // Wasm library build step
        const fastfec_wasm = b.addSharedLibrary("fastfec", null, .unversioned);
        const wasm_target = CrossTarget{ .cpu_arch = .wasm32, .os_tag = .wasi };
        fastfec_wasm.setTarget(wasm_target);
        // Update pcre target for wasm
        pcre.setTarget(wasm_target);
        fastfec_wasm.setBuildMode(mode);
        fastfec_wasm.install();
        fastfec_wasm.linkLibC();
        if (vendored_pcre) {
            fastfec_wasm.addIncludeDir(pcreIncludeDir);
            fastfec_wasm.linkLibrary(pcre);
        } else {
            fastfec_wasm.linkSystemLibrary("libpcre");
        }
        fastfec_wasm.addCSourceFiles(&libSources, &buildOptions);
        fastfec_wasm.addCSourceFile("src/wasm.c", &buildOptions);
    }

    // Test step
    var prev_test_step: ?*std.build.Step = null;
    for (tests) |test_file| {
        const base_file = std.fs.path.basename(test_file);
        const subtest_exe = b.addExecutable(base_file, null);
        subtest_exe.linkLibC();
        subtest_exe.addCSourceFiles(&testIncludes, &buildOptions);
        subtest_exe.addCSourceFile(test_file, &buildOptions);
        // Link PCRE
        if (builtin.os.tag == .windows) {
            subtest_exe.linkSystemLibrary("pcre");
        } else {
            if (vendored_pcre) {
                subtest_exe.addIncludeDir(pcreIncludeDir);
                subtest_exe.linkLibrary(pcre);
            } else {
                subtest_exe.linkSystemLibrary("libpcre");
            }
        }
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
    "deps/pcre/pcre_chartables.c",
    "deps/pcre/pcre_byte_order.c",
    "deps/pcre/pcre_compile.c",
    "deps/pcre/pcre_config.c",
    "deps/pcre/pcre_dfa_exec.c",
    "deps/pcre/pcre_exec.c",
    "deps/pcre/pcre_fullinfo.c",
    "deps/pcre/pcre_get.c",
    "deps/pcre/pcre_globals.c",
    "deps/pcre/pcre_jit_compile.c",
    "deps/pcre/pcre_maketables.c",
    "deps/pcre/pcre_newline.c",
    "deps/pcre/pcre_ord2utf8.c",
    "deps/pcre/pcre_refcount.c",
    "deps/pcre/pcre_string_utils.c",
    "deps/pcre/pcre_study.c",
    "deps/pcre/pcre_tables.c",
    "deps/pcre/pcre_ucd.c",
    "deps/pcre/pcre_valid_utf8.c",
    "deps/pcre/pcre_version.c",
    "deps/pcre/pcre_xclass.c",
};
const pcreIncludeDir = "deps/pcre";
const tests = [_][]const u8{ "src/buffer_test.c", "src/csv_test.c", "src/writer_test.c" };
const testIncludes = [_][]const u8{
    "src/buffer.c",
    "src/memory.c",
    "src/encoding.c",
    "src/csv.c",
    "src/writer.c",
};
const buildOptions = [_][]const u8{
    "-std=c11",
    "-pedantic",
    "-Wall",
    "-W",
    "-Wno-missing-field-initializers",
};
