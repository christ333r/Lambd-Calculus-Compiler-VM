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

enum NodeType {
    FUNCT_NODE, GRUPO_NODE, VAR_NODE, STR_NODE, REF_NODE, NULL_NODE//son los typos de nodos normales. por si acaso los extra no son nesecarios aqui
};

struct Nodo {
    struct StrInt {
        string* Name;
        int IdxStr;
        ~StrInt() {
            delete Name;
        }
    };
    NodeType tipo;
    union Content {
        struct { StrInt Var; Nodo* Body; } Funct;
        struct { Nodo* Left; Nodo* Right; } Grupo;
        StrInt Str;
        StrInt Var;
        StrInt Ref;

        Content() {}
        ~Content() {}
    } Content;

    Nodo() : tipo(NULL_NODE) {}

    ~Nodo() {
        switch (tipo) {
            case FUNCT_NODE:
                delete Content.Funct.Body;
                break;
            case GRUPO_NODE:
                delete Content.Grupo.Left;
                delete Content.Grupo.Right;
                break;
            case VAR_NODE://estos 3 no nesecitan ser liberados
            case REF_NODE:
            case STR_NODE:
            case NULL_NODE://no es ningun tipo
                break;
        }
    }
};

string dump(Nodo* NodePtr) {
    Nodo& Node = *NodePtr;
    NodeType Tipo = Node.tipo;
    string Text = "";
    switch (Tipo) {
        case FUNCT_NODE:
            Text += "f\"";
            Text += *Node.Content.Funct.Var.Name;
            Text += "\".";
            Text += dump(Node.Content.Funct.Body);
            break;
        case GRUPO_NODE:
            Text += "(";
            Text += dump(Node.Content.Grupo.Left);
            Text += ", ";
            Text += dump(Node.Content.Grupo.Right);
            Text += ")";
            break;
        case STR_NODE:
            Text += "\"";
            Text += *Node.Content.Str.Name;
            Text += "\"";
            break;
        case VAR_NODE:
            Text += "\"";
            Text += *Node.Content.Var.Name;
            Text += "\"";
            break;
        case REF_NODE:
            Text += "'";
            Text += *Node.Content.Ref.Name;
            Text += "'";
            break;
        default:
            throw runtime_error("DevError: tipo no definido: " + Tipo);
            break;
    }
    return Text;
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

template<typename M1, typename M2>
using dict = unordered_map<M1,M2>;

struct Compiler {
    string code;
    dict<string, Nodo*> Ast;
    dict<string,int> Strings;
    dict<string,int> referen;
    
    void Deleten_Comentarios() {
        regex VariusComet(R"(/\*[\s\S]*?\*/)");
        code = regex_replace(code,VariusComet,"");
        regex singleComet(R"(//.*)");
        code = regex_replace(code,singleComet,"");
    }

    void compiler() {// la compilacion
        using strings = vector<string>;
        function<Nodo*(string&)> ParseStructs = [&](string& String) -> Nodo* {//parsea las structuras
            cout << "Parseando: " << String << endl;
            strip(String);
            Nodo* NodePtr = new Nodo();
            Nodo& Node = *NodePtr;
            bool isExist = false;
            function<void(string, function<void(const strings&, Nodo&)>,Nodo&)> HandleStruct = [&isExist,&String,&ParseStructs](const string pattern,function<void(const strings&, Nodo&)> callback,Nodo& Node) {//maneja el parseo y la function
                cout << "handle" << endl;
                vector<string> Struct;
                regex Pattern(pattern);
                sregex_iterator it(String.begin(),String.end(),Pattern), end;

                for(int i = 0; i < it->size(); i++) {
                    Struct.push_back((*it)[i].str());
                }
                if (Struct.size() > 1) {
                    if (isExist) {//si isExist es true indica que ya concidio con otra structura anterior a esta
                        throw runtime_error("ErrorDev: hay dos estructuras iguales o similares por favor revisa");
                    }
                    isExist = true;
                    Struct.erase(Struct.begin());//borrar el primer elemento
                    Nodo* NodePtr = new Nodo;
                    Nodo& NodeNew = *NodePtr;
                    callback(Struct, NodeNew);
                    cout << "devuelto Con modificacion" << endl;
                    Node = NodeNew;
                    cout << "dando" << endl;
                }
                cout << "devuelto" << endl;
            };
            HandleStruct(R"(^f\"([^\"]*)\"\.([\s\S]*)$)", [&ParseStructs](const strings& Struct, Nodo& Node) {
                Node.tipo = FUNCT_NODE;
                Node.Content.Funct.Var.Name = new string(Struct[0]);
                string Text = Struct[1];
                Node.Content.Funct.Body = ParseStructs(Text);
            }, Node);
            HandleStruct(R"(^\"([^\"]*)\"$)", [](const strings& Struct, Nodo& Node){
                Node.tipo = STR_NODE;
                Node.Content.Str.Name = new string(Struct[0]);
            },Node);
            HandleStruct(R"(^'([\s\S]*)'$)", [](const strings& Struct, Nodo& Node){
                Node.tipo = REF_NODE;
                Node.Content.Ref.Name = new string(Struct[0]);
            },Node);
            HandleStruct(R"(^\(([\s\S]*\))$)", [&](const strings& Struct, Nodo& Node) {
                Node.tipo = GRUPO_NODE;
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
                            Node.Content.Grupo.Left = ParseStructs(Text);
                            Text = "";
                        }
                    }
                        
                }
                Text.pop_back();
                Node.Content.Grupo.Right = ParseStructs(Text);
            },Node);
            if (isExist) {
                cout << "devolucion" << endl;
                cout << "fin" << endl;
                return NodePtr;
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

    void value(Nodo& Node,unordered_map<string,int> contexto) { //function para optimizar y buscar Errores
        function<int(string*)> NewString = [&](string* StringPtr) -> int{
            string& String = *StringPtr;
            if (Strings.find(String) == Strings.end()) {
                Strings[String] = Strings.size();
                cout << "Strings Set: " << Strings.size() -1 << endl;
                return Strings.size() -1;
            } else {
                cout << "Strings: " << Strings[String] << endl;
                return Strings[String];
            }
        };
        cout << "funct " << endl;
        NodeType key = Node.tipo;
        switch (key) {
            case FUNCT_NODE: {
                unordered_map<string,int> New = contexto;
                int VarIndex = NewString(Node.Content.Funct.Var.Name);
                New[*Node.Content.Funct.Var.Name] = VarIndex;
                Node.Content.Funct.Var.IdxStr = VarIndex;
                value(*Node.Content.Funct.Body, New);
                break;
            }
            case GRUPO_NODE: {
                value(*Node.Content.Grupo.Left, contexto);
                value(*Node.Content.Grupo.Right, contexto);
                break;
            }
            case STR_NODE: {
                if (contexto.find(*Node.Content.Str.Name) != contexto.end()){
                    Node.tipo = VAR_NODE;
                    Node.Content.Var.IdxStr = contexto[*Node.Content.Str.Name];
                } else {
                    Node.Content.Str.IdxStr = NewString(Node.Content.Str.Name);
                }
                break;
            }
            case REF_NODE: {
                if (Ast.find(*Node.Content.Ref.Name) == Ast.end()) {
                    throw runtime_error("Error: function no definida: \"" + *Node.Content.Ref.Name + "\"");
                }
                int index = referen[*Node.Content.Ref.Name];
                Node.Content.Ref.IdxStr = index;
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
        for (auto& [Name, Node] : Ast) {
            cout << "Imprimit "  << Name << "..."<< endl;
            unordered_map<string,int> Context;
            value(*Node, Context);
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
        uleb128(referen.size());
        unordered_map<string,int> Refen;
        cout << "invirtiendo referencias..." << endl;
        for (auto& [key, val] : referen) Refen[key] = val;
        cout << "exportando referencias..." << endl;
        for (auto& [str,_] : referen) {
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
            Nodo* Node = Ast[key];
            cout << "Hola" << endl;
            function<void(Nodo*)> exportFunct = [&exportFunct,&writebits,&uleb128](Nodo* NodePtr) {
                Nodo& Node = *NodePtr;
                NodeType key = Node.tipo;
                switch (key) {
                    case FUNCT_NODE: {
                        writebits({0, 1, 0});
                        uleb128(Node.Content.Funct.Var.IdxStr);
                        exportFunct(Node.Content.Funct.Body);
                        break;
                    }
                    case GRUPO_NODE: {
                        writebits({0, 0,1});
                        exportFunct(Node.Content.Grupo.Left);
                        exportFunct(Node.Content.Grupo.Right);
                        break;
                    }

                    case STR_NODE: {
                        writebits({1, 0, 0});
                        uleb128(Node.Content.Str.IdxStr);
                        break;
                    }

                    case VAR_NODE: {
                        writebits({0, 1, 1});
                        uleb128(Node.Content.Str.IdxStr);
                        break;
                    }

                    case REF_NODE: {
                        writebits({1, 0, 1});
                        uleb128(Node.Content.Ref.IdxStr);
                        break;
                    }
                    
                    default: {
                        throw runtime_error("ErrorDev: key desconocida, Struct:" + dump(NodePtr));
                        break;
                    }
                }
                return;
            };
            exportFunct(Node);
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
    array<int, 3> op;//opciones de compilacion
    if (size > 2) {
        for(int i = 2; i < size; i++) {
            string str = args[i];
            if (str == "-f") {
                op[0] = true;
            } else {
                throw runtime_error("marcador indefinido: " + str + ".");
                return 1;
            }
        }
    }
    string Code_Example;
    if (op[0]) {
        stringstream buffer;
        ifstream fileCode(args[1]);
        if (fileCode.fail()) {
            throw runtime_error("Error: al leer el archivo");
            return 1;
        }
        buffer << fileCode.rdbuf();
        fileCode.close();
        Code_Example = buffer.str();
    }
    Compiler compiler(Code_Example);
    ofstream file("../Text.lame", ios::binary);
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
