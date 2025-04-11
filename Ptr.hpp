#include <functional>
#include <iostream>
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