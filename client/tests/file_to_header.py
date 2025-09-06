import sys
import os

def generate_header(input_file, output_header, name_constant):
    if not os.path.isfile(input_file):
        print(f"Error: '{input_file}' does not exist.")
        return

    with open(input_file, "rb") as f:
        data = f.read()

    with open(output_header, "w") as f:
        # Include guard
        guard = name_constant.upper() + "_H"
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")
        f.write("#include <stdint.h>\n")
        f.write("#include <stddef.h>\n\n")

        # Array definition
        f.write(f"uint8_t {name_constant}[] = {{\n")

        # Write bytes, 12 per line
        for i, byte in enumerate(data):
            f.write(f"0x{byte:02X}")
            if i != len(data) - 1:
                f.write(", ")
            if (i + 1) % 12 == 0:
                f.write("\n")
        f.write("\n};\n\n")

        # Size definition
        f.write(f"size_t {name_constant}_SIZE = {len(data)};\n\n")
        f.write(f"#endif // {guard}\n")

    print(f"Header file '{output_header}' generated successfully.")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f"Usage: python {sys.argv[0]} <input_file> <output_header.h> <name_constant>")
    else:
        generate_header(sys.argv[1], sys.argv[2], sys.argv[3])

