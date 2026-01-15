#include <coev/coev.h>
#include <coev_curl/curl_cli.h>

using namespace coev;

awaitable<void> co_download()
{
    auto filename = "baidu.html";
    auto file = fopen(filename, "wb");
    if (!file)
    {
        LOG_ERR("error opening %s", filename);
        co_return;
    }
    defer(fclose(file));

    CurlCli cli;
    auto curl = cli.get();
    if (!curl)
    {
        LOG_ERR("download error ");
        co_return;
    }
    curl.setopt(CURLOPT_URL, "www.baidu.com");
    curl.setopt(CURLOPT_WRITEDATA, file);
    auto r = co_await curl.action();
    if (r == INVALID)
    {
        LOG_ERR("download error");
    }
    co_return;
}
awaitable<void> co_upload()
{
    auto filename = "baidu.html";
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        LOG_ERR("error opening %s for upload", filename);
        co_return;
    }
    defer(fclose(file));
    if (fseek(file, 0, SEEK_END) != 0)
    {
        LOG_ERR("fseek failed");
        co_return;
    }
    long size = ftell(file);
    if (size == -1)
    {
        LOG_ERR("ftell failed");
        co_return;
    }
    curl_off_t filesize = curl_off_t(size);

    rewind(file);
    CurlCli cli;
    auto curl = cli.get();
    if (curl == nullptr)
    {
        LOG_ERR("upload error ");
        co_return;
    }
    curl.setopt(CURLOPT_URL, "0.0.0.0:80");
    curl.setopt(CURLOPT_READDATA, file);
    curl.setopt(CURLOPT_INFILESIZE_LARGE, filesize);

    co_await curl.action();
}
int main()
{
    set_log_level(LOG_LEVEL_CORE);
    runnable::instance()
        .start(co_download)
        .wait();
    return 0;
}
