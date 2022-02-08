const Builder = @import("std").build.Builder;

pub fn build(b: *Builder) void {
    const exe = b.addExecutable("faeton", null);
    exe.linkLibC();
    exe.addIncludeDir("./lib");
    exe.addCSourceFiles(&.{"./src/main.c", "./lib/lodepng.c"}, 
        &.{
            "-std=c17", 
            "-O0", 
            "-Wall", 
            "-Werror", 
            "-Wno-unused-variable", 
            "-Wno-unused-but-set-variable"
        });

    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    exe.setTarget(target);
    exe.setBuildMode(mode);
    exe.install();

}
