#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>
#include <cctype>

namespace Base64 {

    // 定义标准 Base64 字符集
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    // 判断字符是否为 Base64 字符
    inline bool is_base64(unsigned char c) {
        return (std::isalnum(c) || (c == '+') || (c == '/'));
    }

    // 解码函数：输入为 Base64 编码的字符串，返回原始二进制数据
    inline std::vector<unsigned char> decode(const std::string& encoded_string) {
        int in_len = encoded_string.size();
        int i = 0, j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;

        // 按每 4 个字符一组循环处理
        while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_];
            in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
                char_array_3[0] = (char_array_4[0] << 2) | ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0x0F) << 4) | ((char_array_4[2] & 0x3C) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x03) << 6) | char_array_4[3];
                for (i = 0; i < 3; i++)
                    ret.push_back(char_array_3[i]);
                i = 0;
            }
        }

        // 处理剩余不足 4 个字符的情况
        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;
            for (j = 0; j < 4; j++)
                char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
            char_array_3[0] = (char_array_4[0] << 2) | ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0x0F) << 4) | ((char_array_4[2] & 0x3C) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x03) << 6) | char_array_4[3];
            for (j = 0; j < i - 1; j++) {
                ret.push_back(char_array_3[j]);
            }
        }
        return ret;
    }

} // namespace Base64

#endif // BASE64_H
