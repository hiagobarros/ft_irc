#include <iostream>

int my_atoi(const char* str) {
    if (!str) return 0;

    int i = 0;

    // 1. Ignorar espaços em branco iniciais
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
        ++i;
    }

    // 2. Lidar com o sinal
    int sign = 1;
    if (str[i] == '-') {
        sign = -1;
        ++i;
    } else if (str[i] == '+') {
        ++i;
    }

    // 3. Converter dígitos
    int result = 0;
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        ++i;
    }

    return result * sign;
}

int main() {
    const char* test[] = {
        "123", "-42", "   789", "+56", "12abc34", "abc", "   -0", "0042", "-00123"
    };

    for (int i = 0; i < 9; ++i) {
        std::cout << "my_atoi(\"" << test[i] << "\") = " << my_atoi(test[i]) << std::endl;
    }

    return 0;
}

