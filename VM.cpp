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
struct Ptr {
    void* ptr = nullptr;                 
    function<void(void*)> deleter;
    function<void*(void*)> clonar;
    type_index Type;

    Ptr() : ptr(nullptr), deleter([](void*) {}), clonar([](void*) { return nullptr; }), Type(typeid(nullptr)) {cout << "Ptr inicial"<< endl;}

    template<typename Any>
    Ptr(Any* p) 
        : ptr(p),
          deleter(p ? [](void* ptr) {
            cout << typeid(Any).name() << endl;
            try {
                cout << "value" << endl;
                if (typeid(Any) == typeid(int)) {
                    cout << *static_cast<Any*>(ptr) << endl;
                }
                
                cout << "value end"<< endl;
            } catch (...) {
                cout << "the Type no impribible";
            }
            cout << "delete" << endl;
            try {
                delete static_cast<Any*>(ptr);
            } catch (const exception& E) {
                cout << "Error: " << E.what() << endl;
            } catch (...) {
                cout << "Error UNKNOWN: " << endl;
            }
            
            cout << "delete end" << endl;
        } : [](void*) {}),
          clonar([p](void* ptr) -> void* { return p ? new Any(*static_cast<Any*>(ptr)) : nullptr; }),
          Type(typeid(Any)) {cout << "Ptr Creado de valor: " << *this << endl;}

    template<typename Any>
    Ptr(const Any& p) 
        : ptr(new Any(p)),
          deleter(ptr ? [](void* ptr) { delete static_cast<Any*>(ptr); } : [](void*) {}),
          clonar([p](void* ptr) -> void* { return ptr ? new Any(*static_cast<Any*>(ptr)) : nullptr; }),
          Type(typeid(Any)) {cout << "Ptr Creado: " << *this << endl;}

    Ptr(const Ptr& other) 
        : deleter(other.deleter), clonar(other.clonar), Type(other.Type) {
        ptr = (other.ptr) ? other.clonar(other.ptr) : nullptr;
        cout << "Copia generada de Ptr: " << other << " a Ptr: " << *this  << endl;
    }

    Ptr& operator=(const Ptr& other) {
        cout << "copia de asignacion Ptr: " << other << " a ptr: " << *this << endl;
        if (this != &other) {
            liberar();
            deleter = other.deleter;
            clonar = other.clonar;
            Type = other.Type;
            ptr = (other.ptr) ? other.clonar(other.ptr) : nullptr;
        }
        cout << "copia de asignacion finalizada" << endl;
        return *this;
    }

    Ptr(Ptr&& other) noexcept 
        : ptr(other.ptr), deleter(move(other.deleter)), clonar(move(other.clonar)), Type(move(other.Type)) {
        cout << "movimiento Ptr: " << other << " a ptr: " << *this << endl;
        other.ptr = nullptr; 
        cout << "movimiento finalizada" << endl;
    }

    Ptr& operator=(Ptr&& other) noexcept {
        cout << "movimiento de asignacion Ptr: " << other << " a ptr: " << *this << endl;
        if (this != &other) {
            liberar();
            ptr = other.ptr;
            deleter = move(other.deleter);
            clonar = move(other.clonar);
            Type = move(other.Type);
            other.ptr = nullptr;
        }
        cout << "movimiento de asignacion finalizada" << endl;
        return *this;
    }
    template<typename any>
    void operator=(any other) {
        liberar();
        ptr = new any(other);
        Type = typeid(any);
        deleter = [](void* ptr) { delete static_cast<any*>(ptr); };
        clonar = [](void* ptr) -> void* { return ptr ? new any(*static_cast<any*>(ptr)) : nullptr; };
    }
    template<typename Is>
    bool is() const {
        return Type == typeid(Is);
    }

    template<typename Any>
    Any& get() const {
        cout << "get: " << "Ptr: "<< *this << ", with TypeReturn " << typeid(Any).name() << endl;
        if (Type == typeid(Any) && typeid(Any) == typeid(int)) {
            cout << "value: " << *static_cast<Any*>(ptr) << endl;
        }
        return *static_cast<Any*>(ptr);
    }

    void liberar() {
        if (ptr) {
            cout << "Liberando memoria... Ptr: \"" << *this <<"\"" << "obj: " << this << "\n";
            deleter(ptr);
            ptr = nullptr;
            cout << "Liberación finalizada.\n";
        }
    }

    ~Ptr() {
        liberar();
    }

    friend ostream& operator<<(ostream& os,const Ptr& ptr) {
        os << "{ptr: " << ptr.ptr << ", type: " << ptr.Type.name() << ", obj:" << &ptr << "}";
        return os;
    }
};
using dict = unordered_map<string, Ptr>;
using list = vector<Ptr>;

enum Types {
    FUNCT,
    GRUPO,
    STRING,
    VAR,
    REFERENCIA,
    TUPLE
};

struct VM {
    vector<char> data;
    size_t tell;
    vector<string> Strings;
    vector<list> Functs;
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
        Main = read_uleb128();
        int num_functs = read_uleb128();
        for (int i = 0; i < num_functs; i++) {
            function<list()> parse_function = [&]() {//Parsea una función lambda almacenada en el binario.
                list funct;
                int tag = read_bits(3);
                switch (tag) {
                    case 0b010: {
                        funct.push_back(FUNCT);
                        funct.push_back(read_uleb128());
                        funct.push_back(parse_function());
                        break;
                    }
                    case 0b001: {
                        funct.push_back(GRUPO);
                        funct.push_back(parse_function());
                        funct.push_back(parse_function());
                        break;
                    }
                    case 0b100: {
                        funct.push_back(STRING);
                        funct.push_back(read_uleb128());
                        break;
                    }
                    case 0b011: {
                        funct.push_back(VAR);
                        funct.push_back(read_uleb128());
                        break;
                    }
                    case 0b101: {
                        funct.push_back(REFERENCIA);
                        funct.push_back(read_uleb128());
                        break;
                    }
                    default: {
                        cout<< "tag: " << (tag & 0x3) << (tag & 0x2) << (tag & 0x1) << endl;
                        throw runtime_error("ErrorDev: tag desconocido");
                        break;
                    }
                }
                return funct;
            };
            Functs.push_back(parse_function());
        }
    }
    
    void execute() {//Ejecuta la función `Main`.
        run_function(Functs[Main], {});
    }
    
    void run_function(list& funct,const list& args,unordered_map<int, list> Context = {}) {
        cout << "star" << endl;
        Types& key = funct[0].get<Types>();
        cout << "key: " << key << endl;
        switch (key) {
            case GRUPO:{
                list& left = funct[1].get<list>();
                list& right = funct[2].get<list>();
                cout << "run" << endl;
                run_function(left,{&right}, Context);
                if (left[0].get<Types>() == TUPLE) {
                    funct = left[1].get<list>();
                } else {
                    run_function(right, {}, Context);
                }
                break;
            }
            case REFERENCIA:{
                run_function(Functs[funct[1].get<int>()], args, Context);
                break;
            }
            case VAR:{
                int& var = funct[1].get<int>();
                if (Context.find(var) != Context.end()) {
                    funct = Context[var];
                }
                break;
            }
            case FUNCT:{
                int& var = funct[1].get<int>();
                list& body = funct[2].get<list>();
                unordered_map<int, list> New = Context;//crear una copia
                if (args.size() >= 1){
                    New[var] = args[0].get<list>();
                    run_function(body, {}, New);
                    funct = {TUPLE, body};
                }
                run_function(body, args, Context);
                funct = {FUNCT,var, body};
                break;
            }
            case STRING: {
                break;
            }
            default:{
                cout << "type: " << key;
                throw runtime_error("ErrorDev: typo desconocido");
                break;
            }
        }
        cout << "end" << endl;
    }
    
    string ToText(list& Ast) {
        string Text = "";
        Types& key = Ast[0].get<Types>();
        cout << "key: " << key << endl;
        switch (key) {
            case GRUPO: {
                Text += "(" + ToText(Ast[1].get<list>()) + "," + ToText(Ast[2].get<list>()) + ")";
                break;
            }
            case STRING:
            case VAR: { 
                Text += "\"" + Strings[Ast[1].get<int>()] + "\"";
                break;
            }
            case REFERENCIA:{
                Text += "'" + Ast[1].get<int>();
                Text += "'";
                break;
            }
            case FUNCT: {
                Text += "'f\"" + Strings[Ast[1].get<int>()] + "\"." + ToText(Ast[2].get<list>());
                break;
            }
            default: {
                cout << "typo: " << key << endl;
                throw runtime_error("Error: typo desconocido");
                break;
            }
        }
        cout << "END" << endl;
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
    VM Vm("Text.lame");
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