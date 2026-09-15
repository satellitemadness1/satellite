
#include <string>

void return_strings()
{
    std::string object_bytes_max = "34359738368"; // 32 gb
    std::string threads_max = "1'000'000"; // 1 million threads max
    std::string threads_startup = "256"; // start 256 threads to potentially use
    std::string file_size_max_bytes = object_bytes_max;
    std::string max_memory_bytes = "61847529062"; // -10% of 64gb
    std::string arguments_satc = "arguments.satc=true";
    std::string arguments_satb = "arguments.satb=true";

    std::vector<std::string> return_str = {
        object_bytes_max,
        threads_max,
        file_size_max_bytes,
        max_memory_bytes,
        arguments_satc,
        arguments_satb
    };
}