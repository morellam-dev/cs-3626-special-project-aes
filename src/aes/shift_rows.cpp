#include "aes.h"
#include <algorithm>

namespace aes
{
    Bytes16 shift_rows(const Bytes16 &input, bool inverse)
    {
        Bytes16 output = input; // copy input vals
        // row 2: rotate by 1 (or 3 if going right)
        auto row2 = output.begin() + 4;
        std::rotate(row2, row2 + (!inverse ? 1 : 3), row2 + 4);
        // row 3: rotate by 2
        auto row3 = row2 + 4;
        std::rotate(row3, row3 + 2, row3 + 4);
        // row 4: rotate by 3 (or 1 if going left)
        auto row4 = row3 + 4;
        std::rotate(row4, row4 + (!inverse ? 3 : 1), row4 + 4);
        return output;
    }
}