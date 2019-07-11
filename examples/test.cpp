#include <blobify/load.hpp>
#include <utility>

#include <cstring>
#include <iostream>

namespace Example {

struct ExampleStruct {
    int member1;
    char member2;
};

inline constexpr auto properties(blobify::tag<ExampleStruct>) {
    auto props = blobify::properties_t<ExampleStruct> { };

    props.member<&ExampleStruct::member1>().expected_value = 10;
    props.member<&ExampleStruct::member2>().expected_value = 'c';

    return props;
}

} // namespace Example

struct MemoryBackend {
    char* current;
    char* buffer_end;

    template<size_t N>
    static constexpr MemoryBackend OnArray(char (&array)[N]) {
        return MemoryBackend { array, std::end(array) };
    }

    void load(char* buffer, size_t size) {
        std::memcpy(buffer, current, size);
        current += size;
    }
};

int main() {
    char data[] = { 10, 0, 0, 0, 'c' };

    {
        auto example_value = blobify::load<Example::ExampleStruct>(MemoryBackend::OnArray(data));
        std::cout << "Member 1 is 0x" << std::hex << example_value.member1 << std::endl;
        std::cout << "Member 2 is '" << example_value.member2 << "'" << std::endl;
    }

    data[0] = 11;
    try {
        (void)blobify::load<Example::ExampleStruct>(MemoryBackend::OnArray(data));

        std::cout << "This should not be printed: member1 was not 10 and hence load() should throw an exception!" << std::endl;

    } catch (blobify::unexpected_value_exception<&Example::ExampleStruct::member1>& err) {
        std::cout << "Rightfully caught exception: member 1 was not 10!" << std::endl;
    }
}
