#include <iostream>
#include <string>

// Função que simula c_str(), mas retorna um novo char* com cópia dos dados
char* my_c_str(const std::string& str) {
    // Obtém o tamanho da string
    int len = 0;
    while (true) {
        // Parar quando encontrar o final (não necessário, mas evita depender de str.size())
        // Ou apenas contar usando index
        if (str[len] == '\0') break;
        ++len;
        // Proteção extra (pode ser omitida se confiar em str)
        if (len >= static_cast<int>(str.size())) break;
    }

    // Aloca nova string com espaço para o null terminator
    char* result = new char[len + 1];

    // Copia os caracteres
    for (int i = 0; i < len; ++i) {
        result[i] = str[i];
    }

    // Adiciona o terminador nulo
    result[len] = '\0';

    return result;
}

int main() {
    std::string texto = "Olá, mundo!";
    char* cstr = my_c_str(texto);

    std::cout << "String original: " << texto << std::endl;
    std::cout << "Resultado c_str: " << cstr << std::endl;

    // Libera a memória alocada
    delete[] cstr;

    return 0;
}

