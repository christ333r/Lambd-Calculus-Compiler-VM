# Compilador for lambd-calculus
# by Christopher Chura galvan
# v1.0
# Not Public
import json#solo para visualizar el Functs
import re#para regex
import struct

class Compiler:
    def Deleten_Comentarios(self) -> None:
        code = self.code
        code = re.sub(r"/\*[\s\S]*?\*/","",code)
        code = re.sub(r"/.*","",code)
        self.code = code
        
    def compiler(self) -> None:
        """la compilacion"""
        def ParseStructs(String):
            """parsea las structuras"""
            String = String.strip()
            IsStruct = False
            def HandleStruct(pattern, callback, IsStruct) -> (tuple[list, dict] | None):
                """maneja el parseo y la function"""
                Struct = re.match(pattern, String)
                if Struct:
                    return re.sub(pattern,"", String), callback(Struct.groups())
                return String, IsStruct
            String, IsStruct = HandleStruct(r"^f\"([^\"]*)\"\.([\s\S]*)$", lambda Struct:{
                "type":"funct",
                "var": Struct[0],
                "funct": ParseStructs(Struct[1])[0],
            }, IsStruct)
            String, IsStruct = HandleStruct(r"^\"([^\"]*)\"$", lambda Struct:{
                "type":"String",
                "String": Struct[0],
            },IsStruct)
            String, IsStruct = HandleStruct(r"^'([\s\S]*)'$", lambda Struct:{
                "type":"referecia",
                "referecia": Struct[0],
            },IsStruct)
            def Handle_grupo(Struct):
                Node = {
                    "type":"grupo",
                }
                SubNodes = re.findall("([^(),]*[(),])",Struct[0])
                Depth = 0
                Text = ""
                for subNode in SubNodes:
                    type = subNode[-1] #ultimo elemento
                    Text += subNode
                    if type == "(":
                        Depth += 1
                    if type == ")":
                        Depth -= 1
                    if type == ",":
                        if Depth == 0:
                            Text = Text[0:-1]
                            Node["left"] = ParseStructs(Text)[0]
                            Text = ""
                Node["right"] = ParseStructs(Text[0:-1])[0]
                return Node
                
            String,IsStruct = HandleStruct(r"^\(([\s\S]*\))$", Handle_grupo,IsStruct)
            if IsStruct:
                return IsStruct, String
            raise ValueError(f"Detecte Unknown : \"{String}\"")
            
        
        blocks = re.findall("([^;]*;)",self.code)
        for block in blocks:
            block = block.rstrip().lstrip()
            name, body = re.findall(r"^(\S*)\s*=\s*([^;]*);$",block)[0]
            body = ParseStructs(body)[0]
            self.Ast[name] = body
            self.referen.append(name)
    
    def value(self, funct, contexto:dict) -> None:
        """function para optimizar y buscar Errores"""
        def NewString(string:str):
            if not string in self.String:
                self.String.append(string)
                return len(self.String) -1
            else:
                return self.String.index(string)
        
        if not funct.get("type"):
            raise ValueError(f"ErrorDev: Estructura:{funct} debe tener \"type\"")#Los ErrorDev son para los desarolladores del compilador
        key = funct["type"]
        if key == "grupo":
            self.value(funct["left"], contexto)
            self.value(funct["right"], contexto)
        elif key == "funct":
            New = contexto.copy()
            
            New[funct["var"]] = NewString(funct["var"])
            funct["var"] = NewString(funct["var"])
            self.value(funct["funct"], New)
        elif key == "String":
            if funct["String"] in contexto:
                funct["type"] = "var"
                funct["String"] = contexto[funct["String"]]
            else:
                funct["String"] = NewString(funct["String"])
            
        elif key == "referecia":
            if not funct["referecia"] in self.Ast:
                raise ValueError(f"Error: function no definida: \"{funct["referecia"]}\"")
            funct["referecia"] = self.referen.index(funct["referecia"])
            
    def __init__(self, Code) -> None:
        self.code = Code
        self.Ast = {}
        self.String = []#para todos los strings
        self.referen = []#para indice de las refenrencias
        self.Deleten_Comentarios()
        
        self.compiler()
        if not self.Ast.get("Main"):
            raise ValueError("Error: no existe un main")#Error normal
        for key in list(self.Ast.keys()):
            self.value(self.Ast[key], {})
        
        print(json.dumps(self.Ast,indent=4))
        print(json.dumps(self.String,indent=4))

    def Export(self) -> bytes:
        """Exportar la compilacion"""
        binario = bytearray()
        bit_buffer = 0
        bit_count = 0

        def writebit(bit: bool):
            """escribe un ´bit´"""
            nonlocal bit_buffer, bit_count
            bit_buffer = (bit_buffer << 1) | bit
            bit_count += 1
            if bit_count == 8:
                binario.append(bit_buffer)
                bit_buffer = 0
                bit_count = 0
        
        def writebits(*bits):
            """escribe los `bits` en el archivo"""
            for bit in bits:
                writebit(bit)

        def Pack(fmt: str, value):
            """empaqueta datos de tipo:`fmt` de tamaño: `value` y los escribe automaticamente"""
            packed_data = struct.pack(fmt, value)
            for byte in packed_data:
                for i in range(7, -1, -1):
                    writebit((byte >> i) & 1)
        def uleb128(num:int) -> None:
            """codifica `num`en uleb128 y lo escribe """
            while True:
                byte = num & 0b1111111
                num >>= 7
                byte |= 0b10000000 if num != 0 else 0 
                Pack("B",byte)
                if num == 0:
                    break  
        
        Pack("5s", b'lambd')#texto magic(indicar archivo de tipo lambd)
        uleb128(len(self.String))
        for string in self.String:
            Pack(f"{len(string)}s", string.encode("utf-8"))
            Pack("B", 0)#indicar fin de un string
        uleb128(self.referen.index("Main"))
        uleb128(len(self.Ast.keys()))
        for funct in self.Ast.values():
            def exportFunct(funct):
                key = funct["type"]
                if key == "funct":
                    writebits(0,1,0)
                    uleb128(funct["var"])
                    exportFunct(funct["funct"])
                elif key == "grupo":
                    writebits(0,0,1)
                    exportFunct(funct["left"])
                    exportFunct(funct["right"])
                elif key == "String":
                    writebits(1,0,0)
                    uleb128(funct["String"])
                elif key == "var":
                    writebits(0,1,1)
                    uleb128(funct["String"])
                elif key == "referecia":
                    writebits(1,0,1)
                    uleb128(funct["referecia"])
                else:
                    raise ValueError(f"ErrorDev: key desconocida: {key}, Struct:{funct}")
            exportFunct(funct)
        
        if bit_count > 0:
            binario.append(bit_buffer << (8 - bit_count))

        return bytes(binario) 

Code_Example = """
addOne = f"x".f"f".("f","x");
Main = ('addOne',"y");     
"""

compiler = Compiler(Code_Example)
with open("Text.lame","wb") as file:
    file.write(compiler.Export())