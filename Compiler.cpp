/* 
    Compilador for lambd-calculus
    by Christopher Chura galvan
    v1.0
    Not Public
*/
//import json#solo para visualizar el Functs
#include <regex>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <functional>
#include <array>
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
    bool is() {
        return Type == typeid(Is);
    }

    template<typename Any>
    Any& get() {
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

template<typename type>
string dump(type& Tipo, uint8_t flags) {
    if (flags == 0) {//dict
        for(auto& [key, _] : Tipo) {
            cout << key << " : ";
            Ptr val = Tipo[key];
            string Type = val.Type.name();
            if (Type == "dict") {
                dump(val.get<dict>(), 0);
            } else if (Type == "list") {
                //dump(val.get<list>(), 1);
            } else if (Type == "str") {
                //dump(val.get<string>(), 2);
            } else {
                cout << val;
            }
        }
    } else if (flags == 1) {//list
        /*for(Ptr ptrItem : Tipo) {
            string Type = ptrItem.ticket;
            if (Type == "dict") {
                dump(ptrItem.get<dict>(), 0);
            } else if (Type == "list") {
                dump(ptrItem.get<list>(), 1);
            } else if (Type == "str") {
                dump(ptrItem.get<string>(), 2);
            } else {
                cout << ptrItem;
            }
            cout << ",";
        }*/
    } else if (flags == 2) {//value
        /*cout << Tipo;*/
    } else {
        /*cout << "Flags: " << flags << endl;
        throw runtime_error("ErrorDev: flags no definido");*/
    }
    return "";
}

template<typename type,typename Type2>
string dumpType(type& Tipo, uint8_t flags) {
    if (flags == 0) {//dict
        for(auto& [key, _] : Tipo) {
            cout << key << " : " << Tipo[key] << ", ";
        }
    } else if (flags == 1) {//list
        for(auto& item : Tipo) {
            //cout << item << ", ";
        }
    } else if (flags == 2) {//value

    } else {
        cout << "Flags: " << flags << endl;
        throw runtime_error("ErrorDev: flags no definido");
    }
    return "";
}

void strip(string& str) {
    regex left(R"(^\s+)");
    str = regex_replace(str,left,"");
    regex right(R"(\s+$)");
    str = regex_replace(str,right,"");
}

enum Types{
    FUNCT,
    STRING,
    VAR,
    REFERENCIA,
    GRUPO,
    ERROR
};

struct Compiler {
    string code;
    dict Ast;
    unordered_map<string,int> Strings;
    unordered_map<string,int> referen;
    
    void Deleten_Comentarios() {
        regex VariusComet(R"(/\*[\s\S]*?\*/)");
        code = regex_replace(code,VariusComet,"");
        regex singleComet(R"(//.*)");
        code = regex_replace(code,singleComet,"");
    }

    void compiler() {// la compilacion
        using strings = vector<string>;
        function<Ptr(string&)> ParseStructs = [&](string& String) -> Ptr {//parsea las structuras
            cout << "Parseando: " << String << endl;
            strip(String);
            dict* RealStruct = new dict();
            dict& IsStruct = *RealStruct;
            bool isExist = false;
            function<void(string, function<void(const strings&, dict&)>,dict&)> HandleStruct = [&isExist,&String,&ParseStructs](const string pattern,function<void(const strings&, dict&)> callback,dict& IsStruct) {//maneja el parseo y la function
                cout << "handle" << endl;
                vector<string> Struct;
                regex Pattern(pattern);
                sregex_iterator it(String.begin(),String.end(),Pattern), end;

                for(int i = 0; i < it->size(); i++) {
                    Struct.push_back((*it)[i].str());
                }
                for(string& str : Struct) {
                    cout << "item: "<< str << endl;
                }
                if (Struct.size() > 1) {
                    if (isExist) {//si isExist es true indica que ya concidio con otra structura anterior a esta
                        throw runtime_error("ErrorDev: hay dos estructuras iguales o similares por favor revisa");
                    }
                    isExist = true;
                    Struct.erase(Struct.begin());//borrar el primer elemento
                    dict Node;
                    callback(Struct, Node);
                    cout << "devuelto Con modificacion" << endl;
                    IsStruct = move(Node);
                    cout << "dando" << endl;
                }
                cout << "devuelto" << endl;
            };
            HandleStruct(R"(^f\"([^\"]*)\"\.([\s\S]*)$)", [&ParseStructs](const strings& Struct, dict& Node) {
                Node["type"] = Ptr(FUNCT);
                Node["var"] = Ptr(Struct[0]);
                string Text = Struct[1];
                Node["funct"] = ParseStructs(Text);
            }, IsStruct);
            HandleStruct(R"(^\"([^\"]*)\"$)", [](const strings& Struct, dict& Node){
                Node["type"] = Ptr(STRING);
                Node["String"] = Ptr(Struct[0]);
            },IsStruct);
            HandleStruct(R"(^'([\s\S]*)'$)", [](const strings& Struct, dict& Node){
                Node["type"] = Ptr(REFERENCIA);
                Node["referecia"] = Ptr(Struct[0]);
            },IsStruct);
            HandleStruct(R"(^\(([\s\S]*\))$)", [&](const strings& Struct, dict& Node) {
                Node["type"] = Ptr(GRUPO);
                regex Pattern(R"([^(),]*[(),])");
                string Content = Struct[0];
                sregex_iterator SubNodesIt(Content.begin(),Content.end(), Pattern), end;
                vector<string> SubNodes;
                while (SubNodesIt != end) {
                    SubNodes.push_back(SubNodesIt->str());
                    SubNodesIt++;
                }
                int Depth = 0;
                string Text = "";
                for (string subNode : SubNodes) {
                    char type = subNode[subNode.size()-1];
                    Text += subNode;
                    if (type == '(') Depth += 1;
                    if (type == ')') Depth -= 1;
                    if (type == ',') {
                        if (Depth == 0) {
                            Text.pop_back();
                            Node["left"] = ParseStructs(Text);
                            Text = "";
                        }
                    }
                        
                }
                Text.pop_back();
                Node["right"] = ParseStructs(Text);
            },IsStruct);
            if (isExist) {
                cout << "devolucion" << endl;
                Ptr hola = Ptr(RealStruct);
                cout << "fin" << endl;
                return move(hola);
            }
            throw runtime_error("Detecte Unknown : \"" + String + "\"");
        };
        regex Pattern(R"([^;]*;)");
        sregex_iterator blocksIt(code.begin(), code.end(), Pattern), end;
        vector<string> blocks;
        while (blocksIt != end) {
            blocks.push_back(blocksIt->str());
            blocksIt++;
        }
        for (string block : blocks) {
            strip(block);
            regex Definition(R"(^(\S*)\s*=\s*([^;]*);$)");
            sregex_iterator it(block.begin(), block.end(), Definition), end;
            string body, name;
            if (it != end) {
                body = (*it)[2].str();
                name = (*it)[1].str();
                cout << "bodyName"<< body << " " << name << endl;
            } else {
                throw runtime_error("Error: Bloque invalido: \"" + block + "\"");
            }
            Ast[name] = ParseStructs(body);
            referen[name] = referen.size();
        }
    }

    void value(dict& funct,unordered_map<string,int> contexto) { //function para optimizar y buscar Errores
        function<int(string)> NewString = [&](string String) -> int{
            if (Strings.find(String) == Strings.end()) {
                Strings[String] = Strings.size();
                cout << "Strings Set: " << Strings.size() -1 << endl;
                return Strings.size() -1;
            } else {
                cout << "Strings: " << Strings[String] << endl;
                return Strings[String];
            }
        };
        
        if (funct.find("type") == funct.end()) {
            throw runtime_error("ErrorDev: Estructura: '" + dump(funct, 0) + "' debe tener \"type\"");//Los ErrorDev son para los desarolladores del compilador
        }
        cout << "funct " << endl;
        Types key = funct["type"].get<Types>();
        switch (key) {
            case FUNCT: {
                unordered_map<string,int> New = contexto;
                int VarIndex = NewString(funct["var"].get<string>());
                New[funct["var"].get<string>()] = VarIndex;
                funct["var"] = move(Ptr(VarIndex));
                value(funct["funct"].get<dict>(), New);
                break;
            }
            case GRUPO: {
                value(funct["left"].get<dict>(), contexto);
                value(funct["right"].get<dict>(), contexto);
                break;
            }
            case STRING: {
                if (contexto.find(funct["String"].get<string>()) != contexto.end()){
                    funct["type"] = move(Ptr(VAR));
                    funct["String"] = move(Ptr(contexto[funct["String"].get<string>()]));
                } else {
                    funct["String"] = move(Ptr(NewString(funct["String"].get<string>())));
                }
                break;
            }
            case REFERENCIA: {
                if (Ast.find(funct["referecia"].get<string>()) == Ast.end()) {
                    throw runtime_error("Error: function no definida: \"" + funct["referecia"].get<string>() + "\"");
                }
                int index = referen[funct["referecia"].get<string>()];
                funct["referecia"] = move(Ptr(index));
                break;
            }
            default: {
                throw runtime_error("Error: llave no definida" + key); 
                break;
            }
        }
    }
    
    Compiler(
        string Code
    ) {
        code = Code;
        Deleten_Comentarios();
        cout << "Compilando..." << endl;
        compiler();
        cout << "Compilacion finalizada" << endl;
        cout << "Evaluando..." << endl;
        if (Ast.find("Main") == Ast.end()) {
            throw runtime_error("Error: no existe un main\n");
        }
        for (auto& [Name, referen] : Ast) {
            cout << "Imprimit "  << Name << "..."<< endl;
            unordered_map<string,int> Context;
            value(referen.get<dict>(), Context);
        }
        cout << "Evaluacion finalizada" << endl;
    }
    
    void Export(vector<uint8_t>& Export) {//Exportar la compilacion
        cout << "Exportando..." << endl;
        vector<uint8_t>& binario = Export;
        uint8_t bit_buffer = 0;
        uint8_t bit_count = 0;

        auto writebit = [&binario,&bit_buffer, &bit_count](bool bit) {//escribe un ´bit´
            bit_buffer = (bit_buffer << 1) | bit;
            bit_count += 1;
            if (bit_count == 8) {
                binario.push_back(bit_buffer);
                bit_buffer = 0;
                bit_count = 0;
            }
        };

        auto writebits = [&writebit](vector<bool> bits) {//escribe los `bits` en el archivo
            for (bool bit : bits) {
                writebit(bit);
            }
        };
        auto Pack = [&writebit](const void* Value, size_t size) {//empaqueta datos de tipo:`fmt` de tamaño: `value` y los escribe automaticamente
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(Value);
            for (int j = 0; j < size; j++) {
                for (int i = 7;i != -1; i--) {
                    writebit((bytes[j] >> i) & 1);
                }
            }
        };
        function<void(int)> uleb128 = [&](int num){//codifica `num`en uleb128 y lo escribe
            while (true) {
                uint8_t byte = num & 0b1111111;
                num >>= 7;
                if (num != 0) byte |= 0b10000000;
                Pack(&byte,1);
                if (num == 0) break;
            }
                 
        };
        string magic = "lambd";
        Pack(magic.data(), 5);//texto magic(indicar archivo de tipo lambd)
        uleb128(Strings.size());
        unordered_map<string,int> StringsV;
        cout << "invirtiendo strings..." << endl;
        for (auto& [key, val] : Strings) StringsV[key] = val;
        cout << "exportando strings..." << endl;
        for (auto& [str,_] : StringsV) {
            Pack(str.data(), str.size());
            int temp = 0;
            Pack(&temp, 1);
        }
        uleb128(referen.at("Main"));
        uleb128(Ast.size());
        vector<string> Keys;
        cout << "invirtiendo functiones..." << endl;
        for (auto& [key, val] : Ast) Keys.insert(Keys.begin(),key);
        cout << "size: " << Keys.size() << endl;
        cout << "Exportando functiones..." << endl;
        for (string& key : Keys) {
            Ptr& funct = Ast[key];
            cout << "Hola" << endl;
            function<void(dict&)> exportFunct = [&exportFunct,&writebits,&uleb128](dict& funct) {
                Types key = funct["type"].get<Types>();
                switch (key) {
                    case FUNCT: {
                        writebits({0, 1, 0});
                        uleb128(funct["var"].get<int>());
                        exportFunct(funct["funct"].get<dict>());
                        break;
                    }
                    case GRUPO: {
                        writebits({0, 0,1});
                        exportFunct(funct["left"].get<dict>());
                        exportFunct(funct["right"].get<dict>());
                        break;
                    }

                    case STRING: {
                        writebits({1, 0, 0});
                        uleb128(funct["String"].get<int>());
                        break;
                    }

                    case VAR: {
                        writebits({0, 1, 1});
                        uleb128(funct["String"].get<int>());
                        break;
                    }

                    case REFERENCIA: {
                        writebits({1, 0, 1});
                        uleb128(funct["referecia"].get<int>());
                        break;
                    }
                    
                    default: {
                        throw runtime_error("ErrorDev: key desconocida, Struct:" + dump(funct, 0));
                        break;
                    }
                }
                return;
            };
            exportFunct(funct.get<dict>());
        }
        if (bit_count > 0) {
            binario.push_back(bit_buffer << (8 - bit_count));
        }
        cout << "Exportacion finalizada" << endl;
    }
};

int main(int size, const char* args[]) {
    if (size < 2) {
        throw runtime_error("debe propocionar argumentos");
        return 0;
    }
    array<int, 1> op;//opciones de compilacion
    if (size > 2) {
        for(int i = 2; i < size; i++) {
            string str = args[i];
            if (str == "-f") {
                op[1] = true;
            } else {
                throw runtime_error("marcador indefinido: " + str + ".");
                return 1;
            }
        }
    }
    string Code_Example;
    if (op[1]) {
        stringstream buffer;
        ifstream fileCode("code.lts");
        if (fileCode.fail()) {
            throw runtime_error("Error: al leer el archivo");
            return 1;
        }
        buffer << fileCode.rdbuf();
        fileCode.close();
        Code_Example = buffer.str();
    }
    Compiler compiler(Code_Example);
    ofstream file("Text.lame", ios::binary);
    if (file.fail()) {
        throw runtime_error("Error: al abrir el archivo");
        return 0;
    }
    vector<uint8_t> Data;
    compiler.Export(Data);
    cout << "escribiendo exportacion al archivo Text.lame" << endl;
    file.write(reinterpret_cast<const char*>(Data.data()),Data.size());
    file.close();
    cout << "Compilacion finaliza." << endl;
    return 0;
}
