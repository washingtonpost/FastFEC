const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const fastfec_cli = b.addExecutable("fastfec", null);
    fastfec_cli.setTarget(target);
    fastfec_cli.setBuildMode(mode);
    fastfec_cli.install();

    // Add pcre and curl
    fastfec_cli.linkLibC();
    if (builtin.os.tag == .windows) {
        try fastfec_cli.addVcpkgPaths(.static);
        fastfec_cli.linkSystemLibrary("pcre");
    } else {
        fastfec_cli.linkSystemLibrary("libpcre");
    }
    fastfec_cli.linkSystemLibrary("curl");

    fastfec_cli.addCSourceFiles(&.{
        "src/memory.c",
        "src/urlopen.c",
        "src/encoding.c",
        "src/csv.c",
        "src/writer.c",
        "src/fec.c",
        "src/main.c",
    }, &.{
        "-std=c11",
        "-pedantic",
        "-Wall",
        "-W",
        "-Wno-missing-field-initializers",
    });
}
