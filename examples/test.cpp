#include <blobify/load.hpp>
#include <blobify/memory_storage.hpp>
#include <utility>

#include <cstring>
#include <iostream>

namespace Example {

struct InnerStruct {
    std::uint32_t inner_member;
};

struct ExampleStruct {
    int member1;
    InnerStruct member2;
    char member3;
};

inline constexpr auto properties(blob::tag<ExampleStruct>) {
    auto props = blob::properties_t<ExampleStruct> { };

    props.member<&ExampleStruct::member1>().expected_value = 10;
    props.member<&ExampleStruct::member3>().expected_value = 'c';

    return props;
}

} // namespace Example

int main() {
    char data[256] = { 10, 0, 0, 0, 20, 0, 0, 0, 'c' };

    {
        auto example_value = blob::load<Example::ExampleStruct>(blob::memory_storage::OnArray(data));
        std::cout << "Member 1 is 0x" << std::hex << example_value.member1 << std::endl;
        std::cout << "Member 2 is 0x" << std::hex << example_value.member2.inner_member << std::endl;
        std::cout << "Member 3 is '" << example_value.member3 << "'" << std::endl;
    }

    data[0] = 11;
    try {
        (void)blob::load<Example::ExampleStruct>(blob::memory_storage::OnArray(data));

        std::cout << "This should not be printed: member1 was not 10 and hence load() should throw an exception!" << std::endl;

    } catch (blob::unexpected_value_exception<&Example::ExampleStruct::member1>& err) {
        std::cout << "Rightfully caught exception: member 1 was not 10!" << std::endl;
    }
}
