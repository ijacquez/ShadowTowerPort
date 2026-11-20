import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.List;

/** Run the game under GDB, reading GDB commands from standard input. */
public final class RunGameUnderGdb {
    private static void link(Path directory, String name, Path target) throws IOException {
        Path link = directory.resolve(name);
        if (!Files.exists(link, LinkOption.NOFOLLOW_LINKS)) {
            Files.createSymbolicLink(link, directory.relativize(target));
        } else if (!Files.isSameFile(link, target)) {
            throw new IOException(link + " does not refer to " + target);
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length > 1 || (args.length == 1 && !args[0].equals("--inside-xvfb"))) {
            System.err.println("Usage: java tools/RunGameUnderGdb.java [--inside-xvfb] < commands.gdb");
            System.exit(2);
        }
        Path root = Path.of("").toAbsolutePath().normalize();
        Path scratch = root.resolve("scratchpad");
        Path game = root.resolve("Shadow_Tower");
        Path disc = root.resolve("Shadow_Tower.bin");
        if (!Files.isRegularFile(game) || !Files.isRegularFile(disc)) {
            throw new IOException("Build Shadow_Tower and provide Shadow_Tower.bin in the project root");
        }
        Files.createDirectories(scratch);
        link(scratch, "Shadow_Tower", game);
        link(scratch, "Shadow_Tower.bin", disc);

        List<String> command = new ArrayList<>();
        if (args.length == 0) {
            command.add("xvfb-run");
            command.add("-a");
        } else if (System.getenv("DISPLAY") == null) {
            throw new IOException("--inside-xvfb requires an active DISPLAY");
        }
        Path script = Files.createTempFile(scratch, "gdb-", ".gdb");
        int status;
        try {
            Files.copy(System.in, script, StandardCopyOption.REPLACE_EXISTING);
            if (Files.size(script) == 0) throw new IOException("No GDB commands supplied on standard input");
            command.addAll(List.of("gdb", "-q", "-batch", "-x", script.toString(), "--args", "./Shadow_Tower"));
            ProcessBuilder process = new ProcessBuilder(command).directory(scratch.toFile()).inheritIO();
            process.environment().put("ALSOFT_DRIVERS", "null");
            process.environment().put("LIBGL_ALWAYS_SOFTWARE", "1");
            status = process.start().waitFor();
        } finally {
            Files.deleteIfExists(script);
        }
        System.exit(status);
    }
}
