#include <blobify/blobify.hpp>
#include <blobify/properties.hpp>
#include <utility>

namespace Example {

struct ExampleStruct {
    int member1;
    char member2;
};

static constexpr auto properties(blobify::tag<ExampleStruct>) {
    auto props = blobify::properties_t<ExampleStruct> { };

    props.member<&ExampleStruct::member1>().expected_value = 10;
    props.member<&ExampleStruct::member2>().expected_value = '5';

    return props;
}

} // namespace Example

int main() {
    {
        constexpr auto props = properties(blobify::make_tag<Example::ExampleStruct>);

        static_assert(std::get<0>(props.members).expected_value == 10, "Failed to static_assert properties");
        static_assert(std::get<1>(props.members).expected_value == '5', "Failed to static_assert properties");
    }

    {
        using namespace blobify;
        struct Test { int member; };
        constexpr auto props = properties(blobify::make_tag<Test>);
        static_assert(std::get<0>(props.members).expected_value == std::nullopt, "Failed to static_assert properties");
    }
}
