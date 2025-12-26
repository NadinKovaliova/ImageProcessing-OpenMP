// Тема: Паралельна обробка зображень за допомогою OpenMP

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>    // Для вимірювання часу
#include <omp.h>     // Головний заголовний файл OpenMP
#include <fstream>   // Для роботи з файлами
#include <stdexcept> // Для обробки помилок

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t file_type{ 0x4D42 }; // Сигнатура "BM"
    uint32_t file_size{ 0 };
    uint16_t reserved1{ 0 };
    uint16_t reserved2{ 0 };
    uint32_t offset_data{ 0 };
};

struct BMPInfoHeader {
    uint32_t size{ 0 };
    int32_t width{ 0 };
    int32_t height{ 0 };
    uint16_t planes{ 1 };
    uint16_t bit_count{ 0 };
    uint32_t compression{ 0 };
    uint32_t size_image{ 0 };
    int32_t x_pixels_per_meter{ 0 };
    int32_t y_pixels_per_meter{ 0 };
    uint32_t colors_used{ 0 };
    uint32_t colors_important{ 0 };
};
#pragma pack(pop)

// Структура для зберігання одного пікселя (Синій, Зелений, Червоний)
struct Pixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

// --- Функції для роботи з BMP-файлами ---

// Функція для завантаження 24-бітного BMP-файлу в пам'ять
std::vector<Pixel> loadBMP(const char* filename, BMPInfoHeader& bmp_info_header) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Не вдалося відкрити вхідний файл. Переконайтесь, що 'input.bmp' знаходиться в папці проєкту.");
    }

    BMPFileHeader file_header;
    file.read((char*)&file_header, sizeof(file_header));

    if (file_header.file_type != 0x4D42) {
        throw std::runtime_error("Файл не є BMP зображенням.");
    }

    file.read((char*)&bmp_info_header, sizeof(bmp_info_header));

    if (bmp_info_header.bit_count != 24 || bmp_info_header.compression != 0) {
        throw std::runtime_error("Підтримуються лише 24-бітні BMP без стиснення.");
    }

    file.seekg(file_header.offset_data, file.beg);

    std::vector<Pixel> data(bmp_info_header.width * bmp_info_header.height);
    int padding = (4 - (bmp_info_header.width * 3) % 4) % 4;

    for (int y = 0; y < bmp_info_header.height; ++y) {
        file.read((char*)(data.data() + y * bmp_info_header.width), bmp_info_header.width * 3);
        file.seekg(padding, std::ios::cur);
    }
    return data;
}

// Функція для збереження обробленого зображення у BMP-файл
void saveBMP(const char* filename, const std::vector<Pixel>& data, const BMPInfoHeader& bmp_info_header) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Не вдалося створити вихідний файл.");
    }

    BMPFileHeader file_header;
    file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    file_header.file_size = file_header.offset_data + data.size() * 3;

    file.write((char*)&file_header, sizeof(file_header));
    file.write((char*)&bmp_info_header, sizeof(bmp_info_header));

    int padding = (4 - (bmp_info_header.width * 3) % 4) % 4;
    char padding_data[3] = { 0, 0, 0 };

    for (int y = 0; y < bmp_info_header.height; ++y) {
        file.write((const char*)(data.data() + y * bmp_info_header.width), bmp_info_header.width * 3);
        file.write(padding_data, padding);
    }
}

// --- Основна логіка програми ---

int main() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    try {
        BMPInfoHeader info_header;

        // --- 1. Послідовна обробка ---
        std::cout << "Завантаження зображення для послідовної обробки..." << std::endl;
        auto pixels_seq = loadBMP("input.bmp", info_header);

        std::cout << "Починаємо послідовну обробку (один потік)..." << std::endl;
        auto start_seq = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < pixels_seq.size(); ++i) {
            Pixel& p = pixels_seq[i];
            // Формула для розрахунку яскравості (Luminosity)
            uint8_t gray = static_cast<uint8_t>(0.21 * p.red + 0.72 * p.green + 0.07 * p.blue);
            p.red = gray;
            p.green = gray;
            p.blue = gray;
        }

        auto end_seq = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration_seq = end_seq - start_seq;

        std::cout << "Час послідовного виконання: " << duration_seq.count() << " секунд" << std::endl;
        saveBMP("output_sequential.bmp", pixels_seq, info_header);
        std::cout << "Послідовно оброблене зображення збережено як 'output_sequential.bmp'" << std::endl << std::endl;


        // --- 2. Паралельна обробка ---
        std::cout << "Завантаження зображення для паралельної обробки..." << std::endl;
        auto pixels_par = loadBMP("input.bmp", info_header);

        std::cout << "Починаємо паралельну обробку (використовуючи OpenMP)..." << std::endl;
        auto start_par = std::chrono::high_resolution_clock::now();

        // директива автоматично розпаралелює наступний цикл
#pragma omp parallel for
        for (int i = 0; i < pixels_par.size(); ++i) {
            Pixel& p = pixels_par[i];
            uint8_t gray = static_cast<uint8_t>(0.21 * p.red + 0.72 * p.green + 0.07 * p.blue);
            p.red = gray;
            p.green = gray;
            p.blue = gray;
        }

        auto end_par = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration_par = end_par - start_par;

        std::cout << "Час паралельного виконання: " << duration_par.count() << " секунд" << std::endl;
        saveBMP("output_parallel.bmp", pixels_par, info_header);
        std::cout << "Паралельно оброблене зображення збережено як 'output_parallel.bmp'" << std::endl << std::endl;

        // --- 3. Підсумок ---
        std::cout << "========================================" << std::endl;
        std::cout << "Кількість потоків, використаних OpenMP: " << omp_get_max_threads() << std::endl;
        std::cout << "Прискорення за рахунок паралелізму: " << duration_seq.count() / duration_par.count() << " раз(и)" << std::endl;
        std::cout << "========================================" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Сталася помилка: " << e.what() << std::endl;
        return 1;
    }

    system("pause"); // Щоб консоль не закривалася одразу
    return 0;
}
