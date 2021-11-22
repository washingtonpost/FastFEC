const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.build.Builder) !void {
    b.setPreferredReleaseMode(.ReleaseFast);
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const lib_only: bool = b.option(bool, "lib-only", "Only compile the library") orelse false;

    // Compile pcre
    const pcre = b.addStaticLibrary("pcre", null);
    pcre.setBuildMode(.ReleaseFast);
    pcre.setTarget(target);
    pcre.linkLibC();
    pcre.addIncludeDir(pcreIncludeDir);
    pcre.addCSourceFiles(&pcreSources, &buildOptions);

    // Main build step
    if (!lib_only) {
        const fastfec_cli = b.addExecutable("fastfec", null);
        fastfec_cli.setTarget(target);
        fastfec_cli.setBuildMode(mode);
        fastfec_cli.install();

        // Add curl
        fastfec_cli.linkLibC();
        fastfec_cli.addIncludeDir(pcreIncludeDir);
        fastfec_cli.linkLibrary(pcre);
        if (builtin.os.tag == .windows) {
            fastfec_cli.linkSystemLibrary("ws2_32");
            fastfec_cli.linkSystemLibrary("advapi32");
            fastfec_cli.linkSystemLibrary("crypt32");
            fastfec_cli.linkSystemLibrary("libcurl");
            fastfec_cli.linkSystemLibraryName("zlib");
        } else {
            fastfec_cli.linkSystemLibrary("curl");
        }

        fastfec_cli.addCSourceFiles(&libSources, &buildOptions);
        fastfec_cli.addCSourceFiles(&.{
            "src/urlopen.c",
            "src/main.c",
        }, &buildOptions);
    }

    // Library build step
    const fastfec_lib = b.addSharedLibrary("fastfec", null, .unversioned);
    fastfec_lib.setTarget(target);
    fastfec_lib.setBuildMode(mode);
    fastfec_lib.install();
    fastfec_lib.linkLibC();
    fastfec_lib.addIncludeDir(pcreIncludeDir);
    fastfec_lib.linkLibrary(pcre);
    fastfec_lib.addCSourceFiles(&libSources, &buildOptions);

    // Test step
    var prev_test_step: ?*std.build.Step = null;
    for (tests) |test_file| {
        const base_file = std.fs.path.basename(test_file);
        const subtest_exe = b.addExecutable(base_file, null);
        subtest_exe.linkLibC();
        subtest_exe.addCSourceFiles(&testIncludes, &buildOptions);
        subtest_exe.addCSourceFile(test_file, &buildOptions);
        // Link PCRE
        subtest_exe.addIncludeDir(pcreIncludeDir);
        subtest_exe.linkLibrary(pcre);
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
