#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>
#include <cctype>

namespace Base64 {

    // �����׼ Base64 �ַ���
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    // �ж��ַ��Ƿ�Ϊ Base64 �ַ�
    inline bool is_base64(unsigned char c) {
        return (std::isalnum(c) || (c == '+') || (c == '/'));
    }

    // ���뺯��������Ϊ Base64 ������ַ���������ԭʼ����������
    inline std::vector<unsigned char> decode(const std::string& encoded_string) {
        int in_len = encoded_string.size();
        int i = 0, j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;

        // ��ÿ 4 ���ַ�һ��ѭ������
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

        // ����ʣ�಻�� 4 ���ַ������
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
