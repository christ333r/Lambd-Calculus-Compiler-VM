/*
    VM(virtual machine) for lambd-calculus
    by Christopher Chura galvan
    v1.0
    Not Public
*/
#include <cstdlib>
#include <regex>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <functional>
#include <typeindex>

using namespace std;

enum NodeType {
    FUNCT_NODE, GRUPO_NODE, VAR_NODE, STR_NODE, REF_NODE, NULL_NODE//son los typos de nodos normales
    /*los siguientes son typos extra*/, TUPLE_NODE
};

struct Nodo {
    NodeType tipo;
    union {
        struct { struct { Nodo** Ptr; int Name; } Var; Nodo* Body; } Funct;
        struct { Nodo* Left; Nodo* Right; } Grupo;
        int Str;
        struct { Nodo** Ptr; int Name; } Var;
        int Ref;
        Nodo* tuple;
    } Content;

    Nodo() : tipo(NULL_NODE) {}

    ~Nodo() {
        switch (tipo) {
            case FUNCT_NODE:
                delete Content.Funct.Var.Ptr;
                delete Content.Funct.Body;
                break;
            case GRUPO_NODE:
                delete Content.Grupo.Left;
                delete Content.Grupo.Right;
                break;
            case VAR_NODE:
                delete Content.Var.Ptr;
                break;
            case REF_NODE://estos dos no nesecitan ser liberados
            case STR_NODE:
            case NULL_NODE://no es ningun tipo
            case TUPLE_NODE:// tupla usada de Nodos usanda en funct al hacer reduccion beta
                break;
        }
    }

    Nodo* copy() const {
        Nodo* nuevo = new Nodo;
        nuevo->tipo = tipo;
        switch (tipo) {
            case FUNCT_NODE:
                nuevo->Content.Funct.Var.Name = Content.Funct.Var.Name;
                nuevo->Content.Funct.Body = Content.Funct.Body->copy();
                break;
            case GRUPO_NODE:
                nuevo->Content.Grupo.Left = Content.Grupo.Left->copy();
                nuevo->Content.Grupo.Right = Content.Grupo.Right->copy();
                break;
            case VAR_NODE:
                nuevo->Content.Var.Name = Content.Var.Name;
                break;
            case REF_NODE:
                nuevo->Content.Ref = Content.Ref;
                break;
            case STR_NODE:
                nuevo->Content.Str = Content.Str;
                break;
            case TUPLE_NODE:
                throw runtime_error("ErrorDev: una tupla no es parte de los tipos disponibles de lambd no deberia copiarse");
                break;
            case NULL_NODE:
                throw runtime_error("ErrorDev: verifique codigo por que se intento copiar un null");
                break;
        }
        return nuevo;
    }
};

struct VM {
    vector<char> data;
    size_t tell;
    vector<string> Strings;
    vector<string> reference;
    vector<Nodo*> Functs;
    size_t Main;
    VM(
        string file
    ) : tell(0), Main(0){
        ifstream File(file, ios::binary | ios::ate);
        if (!File.is_open() || !File) {
            throw runtime_error("Error: archivo no encontrado o invalido");
        }
        streamsize Size = File.tellg();
        File.seekg(0, ios::beg);
        data.resize(Size);
        if (!File.read(data.data(),Size)) {
            throw runtime_error("Error: al leer el archivo");
        }
        parse();
    }
    void parse() {//Parsea el archivo binario `.lame`.
        function<int(int)> read_bits = [&](int num_bits){//Lee `num_bits` de la secuencia binaria.
            int value = 0;
            for (int i = 0; i < num_bits; i++) {
                if (tell >= (data.size() * 8)){
                    throw runtime_error("Error: fin del archivo inesperado");
                }
                size_t byte_index = static_cast<int>(tell / 8);
                uint8_t bit_index = 7 - (tell % 8);//siempre estara en el rango 0 - 7
                bool bit = (data[byte_index] >> bit_index) & 1;
                value = (value << 1) | bit;
                tell += 1;
            }
            return value;
        };
        function<vector<char>(int)> read_bytes = [&](int num_bytes) -> vector<char> {//Lee `num_bytes` desde el binario.
            if (static_cast<int>(tell / 8) + num_bytes > data.size()) {
                throw runtime_error("Error: lectura fuera de rango");
            }
            vector<char> Bytes;
            for (int i = 0; i < num_bytes; i++) Bytes.push_back(read_bits(8));
            return Bytes;
        };
        function<int()> read_uleb128 = [&]() -> int {//Lee un número codificado en ULEB128.
            int32_t result = 0;
            size_t shift = 0;
            while (true){
                uint8_t Byte = read_bits(8);
                result |= (Byte & 0x7F) << shift;
                shift += 7;
                if ((Byte & 0x80) == 0){
                    break;
                }
            }
            return result;
        };
        function<vector<char>(size_t)> unPack = [&](size_t size) -> vector<char> {// dame una implementacion Base para esto
            return read_bytes(size);
        };
        string magic = "";
        vector<char> magicV = unPack(5);
        magic += magicV[0];
        magic += magicV[1];
        magic += magicV[2];
        magic += magicV[3];
        magic += magicV[4];
        if (magic != "lambd") {
            throw runtime_error("Error: archivo inválido o corrutop");
        }
        int num_strings = read_uleb128();
        for (int _ = 0; _ < num_strings; _++) {
            string string_data = "";
            while (true) {
                uint8_t Byte = read_bytes(1)[0];
                if (Byte == 0x00){//Fin del string
                    break;
                }
                string_data += static_cast<char>(Byte);
            }
            Strings.push_back(string_data);
        }
        int num_referen = read_uleb128();
        for (int _ = 0; _ < num_referen; _++) {
            string string_data = "";
            while (true) {
                uint8_t Byte = read_bytes(1)[0];
                if (Byte == 0x00){//Fin del string
                    break;
                }
                string_data += static_cast<char>(Byte);
            }
            reference.push_back(string_data);
        }
        Main = read_uleb128();
        int num_functs = read_uleb128();
        for (int i = 0; i < num_functs; i++) {
            function<Nodo*()> parse_function = [&]() {//Parsea una función lambda almacenada en el binario.
                Nodo* nodePtr = new Nodo;
                Nodo& node = *nodePtr;
                int tag = read_bits(3);
                switch (tag) {//se utiliza un sistema de banderas para indetificar el tipo
                    case 0b010: {//funct
                        node.tipo = FUNCT_NODE;
                        node.Content.Funct.Var.Name = read_uleb128();
                        node.Content.Funct.Body = parse_function();
                        break;
                    }
                    case 0b001: {//grupo
                        node.tipo = GRUPO_NODE;
                        node.Content.Grupo.Left = parse_function();
                        node.Content.Grupo.Right = parse_function();
                        break;
                    }
                    case 0b100: {//string
                        node.tipo = STR_NODE;
                        node.Content.Str = read_uleb128();
                        break;
                    }
                    case 0b011: {//variable
                        node.tipo = VAR_NODE;
                        node.Content.Var.Name = read_uleb128();
                        break;
                    }
                    case 0b101: {//referencia
                        node.tipo = REF_NODE;
                        node.Content.Ref = read_uleb128();
                        break;
                    }
                    default: {//Error si se decteta bandera desconocida
                        cout<< "flags: " << (tag & 0x3) << (tag & 0x2) << (tag & 0x1) << endl;
                        throw runtime_error("ErrorDev: flags desconocido");
                        break;
                    }
                }
                return nodePtr;
            };
            Functs.push_back(parse_function());
        }
    }
    
    void execute() {//Ejecuta la función `Main`.
        vector<Nodo*> args;
        Functs[Main] = run_function(Functs[Main], args);
    }
    
    Nodo* run_function(Nodo*& NodoPtr, vector<Nodo*>& args,unordered_map<int, Nodo*> Context = {}) {
        Nodo& Node = *NodoPtr;
        NodeType key = Node.tipo;
        switch (key) {
            case GRUPO_NODE: {//1
                Nodo*& left = Node.Content.Grupo.Left;
                Nodo*& right = Node.Content.Grupo.Right;
                args.push_back(right);
                left = run_function(left,args, Context);
                if (left->tipo == TUPLE_NODE) {
                    NodoPtr = left->Content.tuple;
                } else {
                    args.pop_back();
                    right = run_function(right, args, Context);
                }
                break;
            }
            case REF_NODE:{//4
                NodoPtr = run_function(Functs[Node.Content.Ref], args, Context);
                break;
            }
            case VAR_NODE:{//2
                int& var = Node.Content.Var.Name;
                if (Context.find(var) != Context.end()) {
                    NodoPtr = Context[var];
                }
                break;
            }
            case FUNCT_NODE:{//0
                int& var = Node.Content.Funct.Var.Name;
                Nodo*& body = Node.Content.Funct.Body;
                unordered_map<int, Nodo*> New = Context;//crear una copia
                if (args.size() >= 1){
                    New[var] = args.back();
                    args.pop_back();
                    body = run_function(body, args, New);
                    Nodo* NodeNew = new Nodo;
                    NodeNew->tipo = TUPLE_NODE;
                    NodeNew->Content.tuple = body;
                    NodoPtr = NodeNew;
                }
                body = run_function(body, args, Context);
                break;
            }
            case STR_NODE: {//3
                break;
            }
            case NULL_NODE: {//5
                break;
            }
            default:{
                cout << "type: " << key;
                throw runtime_error("ErrorDev: typo desconocido en run_function");
                break;
            }
        }
        
        return NodoPtr;
    }
    
    string ToText(Nodo*& Ast) {
        string Text = "";
        NodeType& key = Ast->tipo;
        switch (key) {
            case GRUPO_NODE: {
                Text += "(" + ToText(Ast->Content.Grupo.Left) + ", " + ToText(Ast->Content.Grupo.Right) + ")";
                break;
            }
            case STR_NODE: { 
                Text += "\"" + Strings[Ast->Content.Str] + "\"";
                break;
            }
            case VAR_NODE: { 
                Text += "\"" + Strings[Ast->Content.Var.Name] + "\"";
                break;
            }
            case REF_NODE:{
                Text += "'" + Ast->Content.Ref;
                Text += "'";
                break;
            }
            case FUNCT_NODE: {
                Text += "'f\"" + Strings[Ast->Content.Funct.Var.Name] + "\"." + ToText(Ast->Content.Funct.Body);
                break;
            }
            default: {
                cout << "typo: " << key << endl;
                throw runtime_error("Error: typo desconocido en ToText");
                break;
            }
        }
        return Text;
    };

    string getString() {
        return ToText(Functs[Main]);
    }

    friend ostream& operator<<(ostream& os, VM&other) {
        os << other.getString();
        return os;
    }
};

int main() {
    VM Vm("../Text.lame");
    cout << Vm << endl;
    cout << "calculando resultado..." << endl;
    while (true) {
        string previus = Vm.getString();
        Vm.execute();
        string si;
        if (previus == Vm.getString()) {
            break;
        }
        cout << Vm << endl;
        cout << "Continuar? (N/Y)";
        cin >> si;
        if (si == "N") {
            break;
        }
    }
    cout << "resultado final:" << endl;
    cout << Vm << endl;
    return 0;
}