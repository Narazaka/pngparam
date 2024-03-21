#include <cstdlib>
#include <cstdio>
#include <bit>
#include <filesystem>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>

#define PNG_SIGNATURE_SIZE 8
const char png_signature[PNG_SIGNATURE_SIZE] = {(char)0x89, (char)0x50, (char)0x4E, (char)0x47, (char)0x0D, (char)0x0A, (char)0x1A, (char)0x0A};
#define IHDR_SIZE 25
#define CHUNK_TYPE_SIZE 4
#define CHUNK_KEYWORD_SIZE 11
#define CHUNK_ITXT_BEFORE_VALUE_SIZE 4
const char chunk_itxt_before_value_signature[CHUNK_ITXT_BEFORE_VALUE_SIZE] = {0, 0, 0, 0};
#define CHUNK_CRC_SIZE 4

char *get_png_parameters_exif(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    char png_signature_buf[PNG_SIGNATURE_SIZE];
    fread(png_signature_buf, sizeof(char), PNG_SIGNATURE_SIZE, fp);
    if (memcmp(png_signature_buf, png_signature, PNG_SIGNATURE_SIZE) != 0)
    {
        fclose(fp);
        return NULL;
    }
    fseek(fp, IHDR_SIZE, SEEK_CUR);

    uint32_t chunk_length;
    char chunk_type[CHUNK_TYPE_SIZE];
    char chunk_keyword[CHUNK_KEYWORD_SIZE];
    char chunk_itxt_before_value[CHUNK_ITXT_BEFORE_VALUE_SIZE];
    while (true)
    {
        fread(&chunk_length, sizeof(uint32_t), 1, fp);
        // png: big-endian(network byte order), x86: little-endian
        if constexpr (std::endian::native == std::endian::little)
        {
            chunk_length = std::byteswap(chunk_length);
        }
        fread(chunk_type, sizeof(char), CHUNK_TYPE_SIZE, fp);
        if (memcmp(chunk_type, "tEXt", CHUNK_TYPE_SIZE) == 0)
        {
            fread(chunk_keyword, sizeof(char), CHUNK_KEYWORD_SIZE, fp);
            if (memcmp(chunk_keyword, "parameters", CHUNK_KEYWORD_SIZE) == 0)
            {
                auto value_size = chunk_length - CHUNK_KEYWORD_SIZE;
                char *exif_buf = (char *)malloc(value_size + 1);
                exif_buf[value_size] = '\0';
                fread(exif_buf, sizeof(char), value_size, fp);
                fclose(fp);
                return exif_buf;
            }
        }
        else if (memcmp(chunk_type, "iTXt", CHUNK_TYPE_SIZE) == 0)
        {
            fread(chunk_keyword, sizeof(char), CHUNK_KEYWORD_SIZE, fp);
            if (memcmp(chunk_keyword, "parameters", CHUNK_KEYWORD_SIZE) == 0)
            {
                fread(chunk_itxt_before_value, sizeof(char), CHUNK_ITXT_BEFORE_VALUE_SIZE, fp);
                if (memcmp(chunk_itxt_before_value, chunk_itxt_before_value_signature, CHUNK_ITXT_BEFORE_VALUE_SIZE) != 0)
                {
                    fclose(fp);
                    return NULL;
                }
                auto value_size = chunk_length - CHUNK_KEYWORD_SIZE - CHUNK_ITXT_BEFORE_VALUE_SIZE;
                char *exif_buf = (char *)malloc(value_size + 1);
                exif_buf[value_size] = '\0';
                fread(exif_buf, sizeof(char), value_size, fp);
                fclose(fp);
                return exif_buf;
            }
        }
        else if (memcmp(chunk_type, "IEND", CHUNK_TYPE_SIZE) == 0)
        {
            fclose(fp);
            return NULL;
        }
        fseek(fp, chunk_length + CHUNK_CRC_SIZE, SEEK_CUR);
    }
}

void print_png_parameters_exif(const char *filename)
{
    char *exif_buf = get_png_parameters_exif(filename);
    if (exif_buf != NULL)
    {
        printf("> %s\n", filename);
        printf("%s\n", exif_buf);
        free(exif_buf);
    }
}

void print_help()
{
    printf("Usage: pngparam <pngfile>\n");
    printf("Usage: pngparam -d <dir>\n");
    printf("Usage: pngparam -r <dir recursive>\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help();
        return 1;
    }

    if (strcmp(argv[1], "-d") == 0)
    {
        if (argc < 3)
        {
            print_help();
            return 1;
        }
        std::filesystem::directory_iterator root(argv[2]);
        for (const auto &entry : root)
        {
            if (entry.path().extension() == ".png")
            {
                print_png_parameters_exif(entry.path().string().c_str());
            }
        }
    }
    else if (strcmp(argv[1], "-r") == 0)
    {
        if (argc < 3)
        {
            print_help();
            return 1;
        }
        std::filesystem::recursive_directory_iterator root(argv[2]);
        for (const auto &entry : root)
        {
            if (entry.path().extension() == ".png")
            {
                print_png_parameters_exif(entry.path().string().c_str());
            }
        }
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            print_png_parameters_exif(argv[i]);
        }
    }
    return 0;
}
