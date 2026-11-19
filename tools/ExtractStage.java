import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.Locale;

/** Extract FDAT.T stage entries and report geometry coordinates; no asset data is bundled. */
public final class ExtractStage {
    private static int u16(ByteBuffer b, int p) { return Short.toUnsignedInt(b.getShort(p)); }
    private static int u8(ByteBuffer b, int p) { return Byte.toUnsignedInt(b.get(p)); }
    private static void write(Path path, byte[] bytes) throws IOException {
        Files.write(path, bytes, StandardOpenOption.CREATE_NEW);
    }
    public static void main(String[] args) throws Exception {
        if (args.length != 3) {
            System.err.println("Usage: java ExtractStage <FDAT.T> <stage-number> <new-output-directory>");
            System.exit(2);
        }
        int stage = Integer.parseInt(args[1]);
        byte[] file = Files.readAllBytes(Path.of(args[0]));
        ByteBuffer b = ByteBuffer.wrap(file).order(ByteOrder.LITTLE_ENDIAN);
        if (file.length < 4 || stage < 0) throw new IllegalArgumentException("Invalid archive or stage");
        int count = u16(b, 0);
        int first = Math.multiplyExact(stage, 10);
        if (2L + 2L * (count + 1L) > 2048 || first + 10 > count)
            throw new IllegalArgumentException("Stage is outside the archive index");
        Path out = Path.of(args[2]);
        Files.createDirectory(out);
        byte[][] entries = new byte[10][];
        for (int i = 0; i < entries.length; i++) {
            int index = first + i;
            long start = (long) u16(b, 2 + index * 2) * 2048;
            long end = (long) u16(b, 2 + (index + 1) * 2) * 2048;
            if (start < 2048 || start > end || end > file.length)
                throw new IllegalArgumentException("Invalid sector range at index " + index);
            entries[i] = java.util.Arrays.copyOfRange(file, (int) start, (int) end);
            if (start != end) {
                String name = i == 1 ? "entry-01-placements.bin" : i == 5 ? "entry-05-models.rtmd" : String.format(Locale.ROOT, "entry-%02d.bin", i);
                write(out.resolve(name), entries[i]);
            }
            System.out.printf("entry %d: sector %d..%d, %d bytes%n", i, start / 2048, end / 2048, end - start);
        }
        ByteBuffer level = ByteBuffer.wrap(entries[1]).order(ByteOrder.LITTLE_ENDIAN);
        ByteBuffer geom = ByteBuffer.wrap(entries[5]).order(ByteOrder.LITTLE_ENDIAN);
        if (level.limit() < 512 * 12 || geom.limit() < 12) throw new IllegalArgumentException("Missing placement table or geometry");
        if (geom.getInt(0) != 0x41) throw new IllegalArgumentException("Unexpected geometry ID");
        int countModels = geom.getInt(8);
        if (countModels < 0 || 12L + 40L * countModels > geom.limit()) throw new IllegalArgumentException("Invalid geometry table");
        StringBuilder csv = new StringBuilder("model,vertex,x,y,z\n");
        int[] min = {Integer.MAX_VALUE, Integer.MAX_VALUE, Integer.MAX_VALUE};
        int[] max = {Integer.MIN_VALUE, Integer.MIN_VALUE, Integer.MIN_VALUE};
        int vertices = 0;
        for (int model = 0; model < countModels; model++) {
            int header = 12 + 40 * model;
            long start = 12L + Integer.toUnsignedLong(geom.getInt(header));
            long n = Integer.toUnsignedLong(geom.getInt(header + 4));
            if (start > geom.limit() || n > (geom.limit() - start) / 8) throw new IllegalArgumentException("Invalid vertex range for model " + model);
            for (int v = 0; v < n; v++) {
                int p = (int) start + v * 8;
                int x = geom.getShort(p), y = geom.getShort(p + 2), z = geom.getShort(p + 4);
                int[] xyz = {x, y, z};
                for (int a = 0; a < 3; a++) { min[a] = Math.min(min[a], xyz[a]); max[a] = Math.max(max[a], xyz[a]); }
                csv.append(model).append(',').append(v).append(',').append(x).append(',').append(y).append(',').append(z).append('\n');
                vertices++;
            }
        }
        write(out.resolve("geometry-vertices.csv"), csv.toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));
        StringBuilder origins = new StringBuilder("record,model,x,y,z,orientation\n");
        for (int i = 0; i < 512; i++) {
            int p = 12 * i;
            int x = u8(level,p+8) << 12;
            int y = (u8(level,p+9) << 12) + ((u8(level,p+11) - 128) << 7);
            int z = u8(level,p+10) << 12;
            origins.append(i).append(',').append(u8(level,p)).append(',').append(x).append(',').append(y).append(',').append(z).append(',').append(u8(level,p+3) & 3).append('\n');
        }
        write(out.resolve("placement-records.csv"), origins.toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));
        System.out.printf("geometry models: %d; vertices: %d; local bounds X %d..%d Y %d..%d Z %d..%d%n", countModels, vertices, min[0], max[0], min[1], max[1], min[2], max[2]);
        System.out.println("Wrote raw sector-aligned entries and decoded CSVs; placement rows include unused slots.");
    }
}
