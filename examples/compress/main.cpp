
#include <coev/utils/compress/coev_compress.h>
#include <string.h>
#include <iostream>
int lz4_text()
{

    const char *buf = "hello world! testing lz4x!!!";
    std::string compressed;
    coev::compress::lz4Z::Compress(compressed, buf, strlen(buf) + 1);

    std::string decompressed;
    coev::compress::lz4Z::Decompress(decompressed, compressed.data(), compressed.size());

    std::cout << decompressed.data() << std::endl;

    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int snappy_text()
{

    const char *buf = "hello world! testing snappyX!!!";
    std::string compressed;
    coev::compress::snappyZ ::Compress(compressed, buf, strlen(buf) + 1);
    std::string decompressed;
    coev::compress::snappyZ ::Decompress(decompressed, compressed.data(), compressed.size());
    std::cout << decompressed.data() << std::endl;

    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int zstd_text()
{

    const char *buf = "hello world! testing zstdX!!!";
    std::string compressed;
    coev::compress::zstdZ::Compress(compressed, buf, strlen(buf) + 1);
    std::string decompressed;
    coev::compress::zstdZ::Decompress(decompressed, compressed.data(), compressed.size());
    std::cout << decompressed.data() << std::endl;

    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int gzip_text()
{

    const char *buf = "hello world! testing gzipX!!!";
    std::string compressed;
    coev::compress::gzipZ::Compress(compressed, buf, strlen(buf));
    std::string decompressed;
    coev::compress::gzipZ::Decompress(decompressed, compressed.data(), compressed.size());
    std::cout << decompressed.data() << std::endl;
    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int zlib_text()
{

    const char *buf = "hello world! testing zlibX!!!";

    std::string compressed;
    coev::compress::zlibX::Compress(compressed, buf, strlen(buf));
    std::string decompressed;
    coev::compress::zlibX::Decompress(decompressed, compressed.data(), compressed.size());
    std::cout << decompressed.data() << std::endl;
    assert(strcmp(decompressed.data(), buf) == 0);
    return 0;
}
int main()
{

    lz4_text();
    snappy_text();
    zstd_text();
    zlib_text();
    gzip_text();
    return 0;
}