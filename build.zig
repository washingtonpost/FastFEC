const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.build.Builder) !void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    // Main build step
    const fastfec_cli = b.addExecutable("fastfec", null);
    fastfec_cli.setTarget(target);
    fastfec_cli.setBuildMode(mode);
    fastfec_cli.install();

    // Add pcre and curl
    fastfec_cli.linkLibC();
    if (builtin.os.tag == .windows) {
        fastfec_cli.linkSystemLibrary("ws2_32");
        fastfec_cli.linkSystemLibrary("advapi32");
        fastfec_cli.linkSystemLibrary("crypt32");
        fastfec_cli.linkSystemLibrary("pcre");
        fastfec_cli.linkSystemLibrary("libcurl");
        fastfec_cli.linkSystemLibraryName("zlib");
    } else {
        fastfec_cli.linkSystemLibrary("libpcre");
        fastfec_cli.linkSystemLibrary("curl");
    }

    fastfec_cli.addCSourceFiles(&libSources, &buildOptions);
    fastfec_cli.addCSourceFiles(&.{
        "src/urlopen.c",
        "src/main.c",
    }, &buildOptions);

    // Library build step
    const fastfec_lib = b.addSharedLibrary("fastfec", null, .unversioned);
    fastfec_lib.setTarget(target);
    fastfec_lib.setBuildMode(mode);
    fastfec_lib.install();
    fastfec_lib.linkLibC();
    if (builtin.os.tag == .windows) {
        fastfec_lib.linkSystemLibrary("pcre");
    } else {
        fastfec_lib.linkSystemLibrary("libpcre");
    }
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
        if (builtin.os.tag == .windows) {
            subtest_exe.linkSystemLibrary("pcre");
        } else {
            subtest_exe.linkSystemLibrary("libpcre");
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
