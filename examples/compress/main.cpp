
#include <utils/compress/compress.h>
#include <string.h>
#include <iostream>
int lz4_text()
{
    coev::compress::lz4X l4;

    const char *buf = "hello world! testing lz4x!!!";
    std::string compressed;
    l4.compress(compressed, buf, strlen(buf) + 1);

    std::string decompressed;
    l4.decompress(decompressed, compressed.data(), compressed.size());

    std::cout << decompressed.data() << std::endl;

    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int snappy_text()
{
    coev::compress::snappyX s4;

    const char *buf = "hello world! testing snappyX!!!";
    std::string compressed;
    s4.compress(compressed, buf, strlen(buf) + 1);
    std::string decompressed;
    s4.decompress(decompressed, compressed.data(), compressed.size());
    std::cout << decompressed.data() << std::endl;

    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int zstd_text()
{

    coev::compress::zstdX z4;
    const char *buf = "hello world! testing zstdX!!!";
    std::string compressed;
    z4.compress(compressed, buf, strlen(buf) + 1);
    std::string decompressed;
    z4.decompress(decompressed, compressed.data(), compressed.size());
    std::cout << decompressed.data() << std::endl;

    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int gzip_text()
{
    coev::compress::gzipX gz;
    const char *buf = "hello world! testing gzipX!!!";
    std::string compressed;
    gz.compress(compressed, buf, strlen(buf));
    std::string decompressed;
    gz.decompress(decompressed, compressed.data(), compressed.size());
    std::cout << decompressed.data() << std::endl;
    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int zlib_text()
{
    coev::compress::zlibX z;
    const char *buf = "hello world! testing zlibX!!!";

    std::string compressed;
    z.compress(compressed, buf, strlen(buf));
    std::string decompressed;
    z.decompress(decompressed, compressed.data(), compressed.size());
    std::cout << decompressed.data() << std::endl;
    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int main()
{

    // lz4_text();
    // snappy_text();
    // zstd_text();
    // zlib_text();
    gzip_text();
    return 0;
}