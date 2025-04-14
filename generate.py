def es_primo(n):
    if n < 2:
        return False
    for i in range(2, int(n**0.5)+1):
        if n % i == 0:
            return False
    return True
N = 100

def Obtener_Primos(num: int,star: int,prePrimos = []) -> list:
    lista = []
    if num & 1 == 0:#recursion si el numero se puede reducir
        num2 = num >> 1
        lista = Obtener_Primos(num2, star)
        lista.extend(Obtener_Primos(num2, star+num2, lista))
        return lista
    
    if star< 2:
        star = 2
    nums = set(range(star,star+num))
    for p in prePrimos:
        for m in range(max(p*2,(star//p*p) // p*p),star + num, p):
            nums.discard(m)
    while len(nums) != 0:# una function para eso?
        i = nums.pop()
        if es_primo(i):
            lista.append(i)
            lim = star + (num >> 1)
            for j in range(i*2,lim, i):
                nums.discard(j)
    return lista
        
    
print(Obtener_Primos(N, 0))
