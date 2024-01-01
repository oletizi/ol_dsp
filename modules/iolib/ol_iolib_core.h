//
// Created by Orion Letizi on 1/1/24.
//

#ifndef OL_DSP_OL_IOLIB_CORE_H
#define OL_DSP_OL_IOLIB_CORE_H
namespace ol::io {
    // XXX: these should probably live somewhere else. util?
    static std::vector<uint8_t> int64_to_byte_array(int64_t value) {
        std::vector<uint8_t> rv;
        rv.reserve(8);
        for (int i = 0; i < 8; ++i) {
            rv.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
        }
        return rv;
    }

    static int64_t byte_array_to_int64(std::vector<uint8_t> data) {
        int64_t result = 0;
        if (data.size() >= 8) {
            for (int i = 7; i >= 0; --i) {
                result = (result << 8) | data[i];
            }
        }

        return result;
    }
}
#endif //OL_DSP_OL_IOLIB_CORE_H
