#include <cstdlib>
#include <cstdio>
#include <bit>
#include <filesystem>
#include <cstring>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#define PNG_SIGNATURE_SIZE 8
const char png_signature[PNG_SIGNATURE_SIZE] = {(char)0x89, (char)0x50, (char)0x4E, (char)0x47, (char)0x0D, (char)0x0A, (char)0x1A, (char)0x0A};
#define IHDR_SIZE 25
#define CHUNK_TYPE_SIZE 4
#define CHUNK_KEYWORD_SIZE 11
#define CHUNK_ITXT_BEFORE_VALUE_SIZE 4
const char chunk_itxt_before_value_signature[CHUNK_ITXT_BEFORE_VALUE_SIZE] = {0, 0, 0, 0};
#define CHUNK_CRC_SIZE 4

char *get_png_parameters(const char *filename)
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
            chunk_length -= CHUNK_KEYWORD_SIZE;
            if (memcmp(chunk_keyword, "parameters", CHUNK_KEYWORD_SIZE) == 0)
            {
                char *parameters = (char *)malloc(chunk_length + 1);
                parameters[chunk_length] = '\0';
                fread(parameters, sizeof(char), chunk_length, fp);
                fclose(fp);
                return parameters;
            }
        }
        else if (memcmp(chunk_type, "iTXt", CHUNK_TYPE_SIZE) == 0)
        {
            fread(chunk_keyword, sizeof(char), CHUNK_KEYWORD_SIZE, fp);
            chunk_length -= CHUNK_KEYWORD_SIZE;
            if (memcmp(chunk_keyword, "parameters", CHUNK_KEYWORD_SIZE) == 0)
            {
                fread(chunk_itxt_before_value, sizeof(char), CHUNK_ITXT_BEFORE_VALUE_SIZE, fp);
                chunk_length -= CHUNK_ITXT_BEFORE_VALUE_SIZE;
                if (memcmp(chunk_itxt_before_value, chunk_itxt_before_value_signature, CHUNK_ITXT_BEFORE_VALUE_SIZE) != 0)
                {
                    fclose(fp);
                    return NULL;
                }
                char *parameters = (char *)malloc(chunk_length + 1);
                parameters[chunk_length] = '\0';
                fread(parameters, sizeof(char), chunk_length, fp);
                fclose(fp);
                return parameters;
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

void print_png_parameters(const char *filename, bool need_comma)
{
    char *parameters = get_png_parameters(filename);
    if (parameters == NULL) {
        return;
    }
    if (need_comma) printf(",");
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Value filename_value;
    filename_value.SetString(rapidjson::StringRef(filename));
    rapidjson::Value parameters_value;
    parameters_value.SetString(rapidjson::StringRef(parameters));
    doc.AddMember("filename", filename_value, doc.GetAllocator());
    doc.AddMember("parameters", parameters_value, doc.GetAllocator());

    char writeBuffer[65536];
    rapidjson::FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
    rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
    doc.Accept(writer);
    printf("\n");
    free(parameters);
}

void print_help()
{
    printf("Usage: pngparam [--json] <pngfile...>\n");
    printf("Usage: pngparam [--json] -d <dir...>\n");
    printf("Usage: pngparam [--json] -r <recursive dir...>\n");
}

int main(int argc, char *argv[])
{
    int optional_offset = 0;
    bool json_output = false;

    if (argc < 2)
    {
        print_help();
        return 1;
    }

    if (strcmp(argv[1], "--json") == 0)
    {
        optional_offset++;
        json_output = true;
    }

    if (strcmp(argv[1 + optional_offset], "-d") == 0)
    {
        if (argc < 3 + optional_offset)
        {
            print_help();
            return 1;
        }
        if (json_output) printf("[\n");
        bool first = true;
        for (int i = 2 + optional_offset; i < argc; i++)
        {
            std::filesystem::directory_iterator root(argv[i]);
            for (const auto &entry : root)
            {
                if (entry.path().extension() == ".png")
                {
                    print_png_parameters(reinterpret_cast<const char *>(entry.path().u8string().c_str()), json_output && !first);
                    first = false;
                }
            }
        }
        if (json_output) printf("]\n");
    }
    else if (strcmp(argv[1 + optional_offset], "-r") == 0)
    {
        if (argc < 3 + optional_offset)
        {
            print_help();
            return 1;
        }
        if (json_output) printf("[\n");
        bool first = true;
        for (int i = 2 + optional_offset; i < argc; i++)
        {
            std::filesystem::recursive_directory_iterator root(argv[i]);
            for (const auto &entry : root)
            {
                if (entry.path().extension() == ".png")
                {
                    print_png_parameters(reinterpret_cast<const char *>(entry.path().u8string().c_str()), json_output && !first);
                    first = false;
                }
            }
        }
        if (json_output) printf("]\n");
    }
    else
    {
        if (argc < 2 + optional_offset)
        {
            print_help();
            return 1;
        }
        if (json_output) printf("[\n");
        bool first = true;
        for (int i = 1 + optional_offset; i < argc; i++)
        {
            print_png_parameters(argv[i], json_output && !first);
            first = false;
        }
        if (json_output) printf("]\n");
    }
    return 0;
}
