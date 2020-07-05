#include <blobify/blobify.hpp>
#include <blobify/stream_storage.hpp>

#include <fstream>
#include <iostream>
#include <vector>

namespace BMP {

struct Header {
    uint16_t signature;
    // uint16_t implicit_padding

    uint32_t size_bytes;
    uint32_t reserved;
    uint32_t data_offset;
};

constexpr auto properties(blob::tag<Header>) {
    blob::properties_t<Header> props { };

    props.member<&Header::signature>().expected_value = uint16_t { 0x4d42 };

    return props;
}

enum class Compression : uint32_t {
    None      = 0,
    RLE8      = 1,
    RLE4      = 2,
    Bitfields = 3,
    JPEG      = 4,
    PNG       = 5
};

// For simplicity, we only support v4 headers
struct SecondaryHeaderV4 {
    uint32_t header_size;

    uint32_t width, height;

    uint16_t num_planes;

    uint16_t bits_per_pixel;

    Compression compression;

    // there's more data in this header, but we're not interested in it
    std::array<uint8_t, 0x58> unused;
};

constexpr auto properties(blob::tag<SecondaryHeaderV4>) {
    blob::properties_t<SecondaryHeaderV4> props { };

    props.expected_size = 108;

    props.member<&SecondaryHeaderV4::header_size>().expected_value = uint32_t { 108 };

    props.member<&SecondaryHeaderV4::num_planes>().expected_value = uint16_t { 1 }; // As per MSDN

    props.member<&SecondaryHeaderV4::compression>().validate_enum_bounds = true;

    return props;
}

} // namespace BMP

int main(int, char* argv[]) {
    std::fstream file(argv[1], std::ios_base::in);
    if (!file) {
        std::cerr << "Failed to open " << argv[1] << std::endl;
        std::exit(1);
    }

    try {
        blob::istream_storage file_blob { file };

        auto header = blob::load<BMP::Header>(file_blob);
        auto secondary_header = blob::load<BMP::SecondaryHeaderV4>(file_blob);

        std::cout << "Data offset: 0x" << std::hex << header.data_offset << std::dec << std::endl;

        std::cout << "Width: " << secondary_header.width << std::endl;
        std::cout << "Height: " << secondary_header.height << std::endl;
        std::cout << "Compression: " << magic_enum::enum_name(secondary_header.compression) << std::endl;
        std::cout << "Bits per pixel: " << secondary_header.bits_per_pixel << std::endl;

        file_blob.seek(header.data_offset - blob::detail::total_serialized_size<BMP::Header>() - blob::detail::total_serialized_size<BMP::SecondaryHeaderV4>());
        struct RGB24 {
            uint8_t r, g, b;
        };
        auto data = blob::load_many<std::vector<RGB24>>(file_blob, secondary_header.width * secondary_header.height);
        std::cout << "Loaded " << data.size() << " pixels\n";
        for (auto i : { 0u, 1u, 2u }) {
            std::cout << "Pixel " << i << " is (" << +data[i].r << ", " << +data[i].g << ", " << +data[i].b << ")\n";
        }
    } catch (blob::unexpected_value_exception<&BMP::SecondaryHeaderV4::header_size> err) {
        std::cerr << "Unsupported header size " << std::dec << err.actual_value << std::endl;

    } catch (blob::unexpected_value_exception<&BMP::SecondaryHeaderV4::num_planes>) {
        std::cerr << "Invalid number of image planes" << std::endl;

    } catch (blob::invalid_enum_value_exception_for<&BMP::SecondaryHeaderV4::compression>) {
        std::cerr << "Invalid compression mode" << std::endl;
    }
}
