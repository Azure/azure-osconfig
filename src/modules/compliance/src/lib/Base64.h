#ifndef BASE64_H
#define BASE64_H

#include "Result.h"

#include <string>

namespace compliance
{
Result<std::string> Base64Decode(const std::string& input);
}

#endif // BASE64_H
