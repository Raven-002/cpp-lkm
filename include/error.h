#pragma once

enum class ErrorCode : int {
    None        =  0,
    AllocFail   = -12,   // -ENOMEM
    HwHandshake = -5,    // -EIO
    BadState    = -22,   // -EINVAL
};
