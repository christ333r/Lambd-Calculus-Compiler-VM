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
    struct StructVar {Nodo** Ptr; int Name; int ID; };
    union {
        struct {StructVar Var; Nodo* Body; } Funct;
        struct { Nodo* Left; Nodo* Right; } Grupo;
        int Str;
        StructVar Var;
        int Ref;
        Nodo* tuple;
    } Content;

    Nodo() : tipo(NULL_NODE) {}

    ~Nodo() {
        cout << "liberando this: " << this << endl;
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

    Nodo* copy(unordered_map<int, tuple<Nodo**, int>> Context, size_t& countID) {
        Nodo* nuevo = new Nodo;
        nuevo->tipo = tipo;
        switch (tipo) {
            case FUNCT_NODE:{
                nuevo->Content.Funct.Var.ID = countID;
                countID++;
                Nodo** SharedNode = new Nodo*;
                *SharedNode = new Nodo;
                Context[Content.Funct.Var.ID] = {SharedNode, nuevo->Content.Funct.Var.ID};
                nuevo->Content.Funct.Var.Ptr = SharedNode;
                nuevo->Content.Funct.Var.Name = Content.Funct.Var.Name;
                nuevo->Content.Funct.Body = Content.Funct.Body->copy(Context, countID);
                break;
            }
            case GRUPO_NODE:{
                nuevo->Content.Grupo.Left = Content.Grupo.Left->copy(Context, countID);
                nuevo->Content.Grupo.Right = Content.Grupo.Right->copy(Context, countID);
                break;
            }
            case VAR_NODE:{
                if (Context.find(Content.Var.ID) == Context.end()) {//la Variable esta suelta(sin padre function que la defina)
                    nuevo = this;//pasar sin modificar
                    break;
                }
                nuevo->Content.Var.Ptr = get<0>(Context[Content.Var.ID]);
                nuevo->Content.Var.ID = get<1>(Context[Content.Var.ID]);
                nuevo->Content.Var.Name = Content.Var.Name;
                break;
            }
            case REF_NODE:{
                nuevo->Content.Ref = Content.Ref;
                break;
            }
            case STR_NODE:{
                nuevo->Content.Str = Content.Str;
                break;
            }
            case TUPLE_NODE:{
                throw runtime_error("ErrorDev: una tupla no es parte de los tipos disponibles de lambd no deberia copiarse");
                break;
            }
            case NULL_NODE:{
                throw runtime_error("ErrorDev: verifique codigo por que se intento copiar un null");
                break;
            }
            default: {
                cout << "tipo:" << tipo << endl;
                throw runtime_error("ErrorDev:Tipo desconocido, intento de copia de tipo desconocido");
            }
        }
        return nuevo;
    }

    friend ostream& operator<<(ostream& os, Nodo& Node) {
        os << "{ tipo:"<< Node.tipo << ", ";
        switch (Node.tipo) {
            case FUNCT_NODE:{
                os << "VarIdx: "<<Node.Content.Funct.Var.Name <<", body: " << *Node.Content.Funct.Body;
                break;
            }
            case GRUPO_NODE:{
                os << "left: "<<*Node.Content.Grupo.Left<<", right: " << *Node.Content.Grupo.Right;
                break;
            }
            case VAR_NODE:{
                os << "VarIdx: "<<Node.Content.Var.Name;
                break;
            }
            case STR_NODE:{
                os << "StrIdx: "<<Node.Content.Str;
                break;
            }
            case REF_NODE:{
                os << "Ref: "<<Node.Content.Ref;
                break;
            }
            case NULL_NODE:{
                os << "NULL";
                break;
            }
            case TUPLE_NODE:{
                os << "TUPLE: "<< *Node.Content.tuple;
                break;
            }
            default: {
                os << "ErrorDev: Tipo no esta en definitiones";
                break;
            }
        }
        os << " }";
        return os;
    }
};

struct VM {
    vector<char> data;
    size_t tell;
    size_t countID = 0;
    vector<string> Strings;
    vector<string> reference;
    vector<Nodo*> Functs;
    size_t Main;
    signed char Debug;
    VM(
        string file, signed char debug
    ) : tell(0), Main(0), Debug(debug) {
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
        int num_functs = read_uleb128();
        Functs.resize(num_functs);
        reference.resize(num_functs);
        for (int i = 0; i < num_functs; i++) {
            struct Tuple{
                Nodo** Node;
                int ID;
                void operator=(Tuple&& other) {
                    Node =  other.Node;
                    ID = other.ID;
                }

                void operator=(const Tuple& other) {
                    Node =  other.Node;
                    ID = other.ID;
                }

                void operator=(const tuple<Nodo**, int>& other) {
                    Node = get<0>(other);
                    ID = get<1>(other);
                }

                string GetText() {
                    stringstream ss;
                    ss << "( " << Node << ", " << ID << " )";
                    return ss.str();
                }
            };
            unordered_map<int, Tuple> ContextEmpty;
            function<Nodo*(unordered_map<int, Tuple>&)> parse_function = [&](unordered_map<int, Tuple>& Context) {//Parsea una función lambda almacenada en el binario.
                
                Nodo* nodePtr = new Nodo;
                Nodo& node = *nodePtr;
                int tag = read_bits(3);
                switch (tag) {//se utiliza un sistema de banderas para indetificar el tipo
                    case 0b010: {//funct
                        node.tipo = FUNCT_NODE;
                        Nodo** sharedNode = new Nodo*;
                        *sharedNode = new Nodo;
                        int ID = countID;
                        countID++;
                        node.Content.Funct.Var.Ptr = sharedNode;
                        node.Content.Funct.Var.Name = read_uleb128();
                        int& idx = node.Content.Funct.Var.Name;
                        bool BPreviousTuple = false;
                        Tuple PreviousTuple;
                        if (Context.find(idx) != Context.end()) {
                            PreviousTuple = Context[idx];
                            BPreviousTuple = true;
                        }
                        node.Content.Funct.Var.ID = ID;
                        tuple New = {sharedNode, ID};
                        Context[idx] = New;
                        node.Content.Funct.Body = parse_function(Context);
                        if (BPreviousTuple) {
                            Context[idx] = PreviousTuple;
                            break;
                        }
                        Context.erase(idx);
                        break;
                    }
                    case 0b001: {//grupo
                        node.tipo = GRUPO_NODE;
                        node.Content.Grupo.Left = parse_function(Context);
                        node.Content.Grupo.Right = parse_function(Context);
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
                        node.Content.Var.Ptr = Context[node.Content.Var.Name].Node;
                        node.Content.Var.ID = Context[node.Content.Var.Name].ID;
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
            string name_funct = "";
            int idx = read_uleb128();
            while (true) {
                uint8_t Byte = read_bytes(1)[0];
                if (Byte == 0x00){//Fin del string
                    break;
                }
                name_funct += static_cast<char>(Byte);
            }
            if (name_funct == "Main") {
                Main = idx;
            }
            
            reference[idx] = name_funct;
            Functs[idx] = parse_function(ContextEmpty);
        }
    }
    
    void execute() {//Ejecuta la función `Main`.
        vector<Nodo*> args;
        run_function(Functs[Main], args);
    }
    
    void run_function(Nodo*& NodoPtr, vector<Nodo*>& args, bool visita = false) {
        Nodo& Node = *NodoPtr;
        NodeType key = Node.tipo;
        switch (key) {
            case GRUPO_NODE: {//1
                Nodo*& left = Node.Content.Grupo.Left;
                Nodo*& right = Node.Content.Grupo.Right;
                if (visita) {
                    vector<Nodo*> voidleft;
                    vector<Nodo*> voidright;
                    run_function(left, voidleft, true);
                    run_function(right, voidright, true);
                    break;
                }
                vector<Nodo*> voidargs;
                run_function(right, voidargs, true);
                if (left->tipo == FUNCT_NODE) {
                    args.push_back(right);
                }
                run_function(left,args);
                if (left->tipo == TUPLE_NODE) {
                    NodoPtr = left->Content.tuple;
                }
                break;
            }
            case REF_NODE:{//4
                if (visita) break;
                Nodo* NodoFunct = Functs[Node.Content.Ref]->copy({},countID);//crear una copia para evitar auto referencia
                run_function(NodoFunct, args);
                NodoPtr = NodoFunct;
                break;
            }
            case VAR_NODE:{//2
                Nodo**& var = Node.Content.Var.Ptr;
                if (visita && (**var).tipo != NULL_NODE) {
                    NodoPtr = (*var)->copy({},countID);
                    break;
                }
                if ((**var).tipo != NULL_NODE) {
                    NodoPtr = (*var)->copy({},countID);
                }
                break;
            }
            case FUNCT_NODE:{//0
                Nodo**& var = Node.Content.Funct.Var.Ptr;
                Nodo*& body = Node.Content.Funct.Body;
                if (args.size() >= 1 && !visita){
                    *var = args.back();
                    args.pop_back();
                    run_function(body, args);
                    Nodo* NodeNew = new Nodo;
                    NodeNew->tipo = TUPLE_NODE;
                    NodeNew->Content.tuple = body;
                    NodoPtr = NodeNew;
                    break;
                }
                vector<Nodo*> Void;
                run_function(body, Void);
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
    }
    
    string ToText(Nodo*& Ast) {
        stringstream Text;
        NodeType& key = Ast->tipo;
        switch (key) {
            case GRUPO_NODE: {
                Text << "(" + ToText(Ast->Content.Grupo.Left) << ", " << ToText(Ast->Content.Grupo.Right) + ")";
                break;
            }
            case STR_NODE: {
                Text << "\"" << Strings[Ast->Content.Str] << "\"";
                break;
            }
            case VAR_NODE: {
                switch (Debug) {
                    case -1:
                        Text << "\"" << Strings[Ast->Content.Var.Name] << "\"";
                        break;
                    case 0:
                        Text << "\"" << Ast->Content.Var.ID << "\"";
                        break;
                    case 1:
                        Text << "(" << **Ast->Content.Var.Ptr << ")";
                    case 2:
                        Text << "(" << **Ast->Content.Var.Ptr << "(" << *Ast->Content.Var.Ptr << "(" << Ast->Content.Var.Ptr << ")))";
                    default:
                        Text << "\"" << Strings[Ast->Content.Var.Name] << "\"";
                        break;
                }
                break;
            }
            case REF_NODE:{
                Text << "'" << reference[Ast->Content.Ref] << "'";
                break;
            }
            case FUNCT_NODE: {
                Text << "f";
                switch (Debug) {
                    case -1:
                        Text << "\"" << Strings[Ast->Content.Funct.Var.Name] << "\"";
                        break;
                    case 0:
                        Text << "(" << Ast->Content.Funct.Var.ID << ")" ;
                        break;
                    case 1:
                        Text << "(" << **Ast->Content.Funct.Var.Ptr << ")";
                    case 2:
                        Text << "(" << **Ast->Content.Funct.Var.Ptr << "(" << *Ast->Content.Funct.Var.Ptr << "(" << Ast->Content.Funct.Var.Ptr << ")))";
                    case 3:
                        Text << "(" << Ast->Content.Funct.Var.ID << "(" << **Ast->Content.Funct.Var.Ptr << "(" << *Ast->Content.Funct.Var.Ptr << "(" << Ast->Content.Funct.Var.Ptr << "))))";
                    case 4:
                        Text << "(" << Ast->Content.Funct.Var.ID << "(" << **Ast->Content.Funct.Var.Ptr << "))";
                    default:
                        Text << "\"" << Strings[Ast->Content.Funct.Var.Name] << "\"";
                        break;
                }
                Text << "." << ToText(Ast->Content.Funct.Body);
                break;
            }
            case TUPLE_NODE: {
                cout << *Ast->Content.tuple << endl;
                throw runtime_error("ErrorDev: se decteto un TUPLE_NODE, desarollador verifique el que todo este correctamente");
                break;
            }
            default: {
                cout << "typo: " << key << "ast:" << *Ast << endl;
                cout << "Functs: " << endl;
                for(Nodo*& node : Functs) {
                    cout << ", "<< *node << endl;
                }
                
                throw runtime_error("ErrorDev: typo desconocido en ToText");
                break;
            }
        }
        return Text.str();
    };

    string getString() {
        return ToText(Functs[Main]);
    }
    
    vector<array<string,2>> getAllFuncts() {
        vector<array<string,2>> FunctsText;
        int i = 0;
        for(Nodo*& Node : Functs) {
            FunctsText.push_back({reference[i], ToText(Node)});
            i++;
        }
        return FunctsText;
    }

    friend ostream& operator<<(ostream& os, VM&other) {
        os << other.getString();
        return os;
    }
};

int main(int size, const char* args[]) {
    if (size < 2) {
        throw runtime_error("debe propocionar argumentos");
        return 0;
    }
    array<int, 3> op;//opciones de VM
    signed Debug = -1;
    if (size > 2) {
        for(int i = 2; i < size; i++) {
            string str = args[i];
            if (str == "-d") {//debug
                op[0] = true;
                i++;
                string debug = args[i];
                Debug = debug == "-1" ? -1 : debug == "0" ? 0 : debug == "1" ? 1 : debug == "2" ? 2 : debug == "3" ? 3 : -1;
            } else {
                throw runtime_error("marcador indefinido: " + str + ".");
                return 1;
            }
        }
    }
    VM Vm(args[1],Debug);
    cout << Vm << endl;
    cout << "Functiones definidas:" << endl;
    for(array<string, 2>& str : Vm.getAllFuncts()) {
        cout << str[0] << ": " << str[1] << endl;
    }
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
    cout << "wait.";
    string NoUsing;
    cin >> NoUsing;
    return 0; 
}