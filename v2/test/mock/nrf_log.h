#define NRF_LOG_INFO(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while(0);
