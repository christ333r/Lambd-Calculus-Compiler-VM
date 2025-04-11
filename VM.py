# VM(virtual machine) for lambd-calculus
# by Christopher Chura galvan
# v1.0
# Not Public
import json#para mostrar la representation
import struct
import copy

class VM:
    def __init__(self, file: str):
        try:
            with open(file, "rb") as File:
                self.data = File.read()
        except FileNotFoundError:
            raise ValueError("Error: archivo no encontrado")
        
        self.tell = 0
        self.Strings = []
        self.Functs = []
        self.Main = 0
        self.parse()

    def parse(self):
        def read_bits(num_bits: int) -> int:
            """Lee `num_bits` de la secuencia binaria."""
            value = 0
            for _ in range(num_bits):
                if self.tell >= len(self.data) * 8:
                    raise ValueError("Error: fin del archivo inesperado")
                byte_index = self.tell // 8
                bit_index = 7 - (self.tell % 8)
                bit = (self.data[byte_index] >> bit_index) & 1
                value = (value << 1) | bit
                self.tell += 1
            return value

        def read_bytes(num_bytes: int) -> int:
            """Lee `num_bytes` desde el binario."""
            if self.tell // 8 + num_bytes > len(self.data):
                raise ValueError("Error: lectura fuera de rango")
            return bytearray([read_bits(8) for _ in range(num_bytes)])

        def read_uleb128() -> int:
            """Lee un número codificado en ULEB128."""
            result = 0
            shift = 0
            while True:
                byte = read_bits(8)
                result |= (byte & 0x7F) << shift
                shift += 7
                if (byte & 0x80) == 0:
                    break
            return result
        
        def unPack(fml:str, size):
            return struct.unpack(fml, read_bytes(size))[0]
        
        """Parsea el archivo binario `.lame`."""
        if unPack("5s", 5) != b"lambd":
            raise ValueError("Error: archivo inválido o corrutop")

        num_strings = read_uleb128()
        for _ in range(num_strings):
            string_data = b""
            while True:
                byte = read_bytes(1)
                if byte == b"\x00":  # Fin del string
                    break
                string_data += byte
            self.Strings.append(string_data.decode("utf-8","ignore"))
        self.Main = read_uleb128()
        num_functs = read_uleb128()
        for i in range(num_functs):
            def parse_function():
                """Parsea una función lambda almacenada en el binario."""
                funct = []
                tag = read_bits(3)
                if tag == 0b010:
                    funct.append("funct")
                    funct.append(read_uleb128())
                    funct.append(parse_function())
                elif tag == 0b001:
                    funct.append("grupo")
                    funct.append(parse_function())
                    funct.append(parse_function())
                elif tag == 0b100:
                    funct.append("string")
                    funct.append(read_uleb128())
                elif tag == 0b011:
                    funct.append("var")
                    funct.append(read_uleb128())
                elif tag == 0b101:
                    funct.append("referencia")
                    funct.append(read_uleb128())
                else:
                    raise ValueError(f"ErrorDev: tag desconocido:{bin(tag)}")
                return funct
                
            self.Functs.append(parse_function())

    
    def execute(self):
        """Ejecuta la función `Main`."""
        self.Functs[self.Main] = self.run_function(self.Functs[self.Main])

    def run_function(self, funct, *args, Context = {}):
        key, *functs = funct
        if key == "grupo":
            left, right = functs
            funct[1] = self.run_function(left, right, Context=Context)
            if isinstance(funct[1], tuple):
                funct = funct[1][0]
            else:
                funct[2] = self.run_function(right, Context=Context)
        elif key == "referencia":
            return self.run_function(copy.deepcopy(self.Functs[functs[0]]), *args, Context=Context)
        elif key == "var":
            var = functs[0]
            if var in Context:
                funct = Context[var]
                
        elif key == "funct":
            var, body = functs
            new = Context.copy()
            if len(args) >= 1:
                new[var] = args[0]
                body = self.run_function(body, Context = new)
                return body,True
            return ["funct",var, self.run_function(body, *args, Context = Context)]
        elif key == "string":
            pass
        else:
            raise ValueError(f"ErrorDev: typo desconocido:{key}")
        return funct

    def __repr__(self):
        Text = ""
        def ToText(ast):
            key, *funct = ast
            if key == "grupo":
                return f"({ToText(funct[0])},{ToText(funct[1])})"
            if key == "string":
                return f'"{self.Strings[funct[0]]}"'
            if key == "var":
                return f'{self.Strings[funct[0]]}'
            if key == "referencia":
                return f"'{funct[0]}'"
            if key == "funct":
                return f'f"{self.Strings[funct[0]]}".{ToText(funct[1])}'
            else:
                raise ValueError(f"Error: typo desconocido: {key}")
        
        if True:
            Texts = []
            for item in self.Functs:
                Texts.append(ToText(item))
            Text = Texts
        else:
            Text = ToText(self.Functs[self.Main])
        return Text


Vm = VM("Text.lame")
print(Vm.__repr__())
Vm.execute()
Vm.execute()
Vm.execute()
Vm.execute()
Vm.execute()
Vm.execute()
print("resultado:")
print(Vm.__repr__())