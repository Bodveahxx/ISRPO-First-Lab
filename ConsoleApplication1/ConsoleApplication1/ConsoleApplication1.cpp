#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <cstring>
#include <cwchar>

using namespace std;

// 1) strreplace: заменить ВСЕ вхождения oldSubstr на newSubstr

// ANSI
BOOL strreplace(char* str, const char* oldSubstr, const char* newSubstr, DWORD bufferSize)
{
    if (!str || !oldSubstr || !newSubstr || bufferSize == 0) return FALSE;

    size_t lenOld = strlen(oldSubstr);
    size_t lenNew = strlen(newSubstr);
    if (lenOld == 0) return FALSE;

    char* pos = str;
    while ((pos = strstr(pos, oldSubstr)) != NULL)
    {
        size_t tailLen = strlen(pos + lenOld);
        size_t newTotalLen = (pos - str) + lenNew + tailLen;

        if (newTotalLen + 1 > bufferSize) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        memmove(pos + lenNew, pos + lenOld, tailLen + 1);
        memcpy(pos, newSubstr, lenNew);

        pos += lenNew;
    }

    return TRUE;
}

// Unicode
BOOL strreplace(wchar_t* str, const wchar_t* oldSubstr, const wchar_t* newSubstr, DWORD bufferSize)
{
    if (!str || !oldSubstr || !newSubstr || bufferSize == 0) return FALSE;

    size_t lenOld = wcslen(oldSubstr);
    size_t lenNew = wcslen(newSubstr);
    if (lenOld == 0) return FALSE;

    wchar_t* pos = str;
    while ((pos = wcsstr(pos, oldSubstr)) != NULL)
    {
        size_t tailLen = wcslen(pos + lenOld);
        size_t newTotalLen = (pos - str) + lenNew + tailLen;

        if (newTotalLen + 1 > bufferSize) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        memmove(pos + lenNew, pos + lenOld, (tailLen + 1) * sizeof(wchar_t));
        memcpy(pos, newSubstr, lenNew * sizeof(wchar_t));

        pos += lenNew;
    }

    return TRUE;
}

// 2) parsecsventry: извлечь n-е поле CSV 

// ANSI
BOOL parsecsventry(const char* csvLine, int fieldIndex, char* outputBuffer, DWORD bufferSize)
{
    if (!csvLine || !outputBuffer || bufferSize == 0 || fieldIndex < 0) return FALSE;

    int curField = 0;
    bool inQuotes = false;
    DWORD outPos = 0;

    for (DWORD i = 0;; i++)
    {
        char c = csvLine[i];

        if (c == '\0' || (c == ',' && !inQuotes))
        {
            if (curField == fieldIndex) {
                if (outPos >= bufferSize) {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }
                outputBuffer[outPos] = '\0';
                return TRUE;
            }
            curField++;
            outPos = 0;
            inQuotes = false;
            if (c == '\0') break;
            continue;
        }

        if (curField != fieldIndex) {
            if (c == '"') inQuotes = !inQuotes;
            continue;
        }

        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }

        if (outPos + 1 >= bufferSize) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        outputBuffer[outPos++] = c;
    }

    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
}

// Unicode
BOOL parsecsventry(const wchar_t* csvLine, int fieldIndex, wchar_t* outputBuffer, DWORD bufferSize)
{
    if (!csvLine || !outputBuffer || bufferSize == 0 || fieldIndex < 0) return FALSE;

    int curField = 0;
    bool inQuotes = false;
    DWORD outPos = 0;

    for (DWORD i = 0;; i++)
    {
        wchar_t c = csvLine[i];

        if (c == L'\0' || (c == L',' && !inQuotes))
        {
            if (curField == fieldIndex) {
                if (outPos >= bufferSize) {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }
                outputBuffer[outPos] = L'\0';
                return TRUE;
            }
            curField++;
            outPos = 0;
            inQuotes = false;
            if (c == L'\0') break;
            continue;
        }

        if (curField != fieldIndex) {
            if (c == L'"') inQuotes = !inQuotes;
            continue;
        }

        if (c == L'"') {
            inQuotes = !inQuotes;
            continue;
        }

        if (outPos + 1 >= bufferSize) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        outputBuffer[outPos++] = c;
    }

    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
}

// 3) getstringencoding: 1=ANSI, 2=UTF-16LE, 3=UTF-8, 0=неизвестно

static bool has_utf8_pattern(const unsigned char* s, int len)
{
    int i = 0;
    bool multibyte = false;

    while (i < len) {
        unsigned char c = s[i];
        if (c <= 0x7F) { i++; continue; }

        multibyte = true;
        int bytes = 0;

        if ((c & 0xE0) == 0xC0) bytes = 2;
        else if ((c & 0xF0) == 0xE0) bytes = 3;
        else if ((c & 0xF8) == 0xF0) bytes = 4;
        else return false;

        if (i + bytes > len) return false;
        for (int k = 1; k < bytes; k++) {
            if ((s[i + k] & 0xC0) != 0x80) return false;
        }
        i += bytes;
    }
    return multibyte;
}

// ANSI
int getstringencoding(const char* str, int length)
{
    if (!str || length <= 0) return 0;
    const unsigned char* s = (const unsigned char*)str;

    if (length >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) return 3; // UTF-8 BOM
    if (length >= 2 && s[0] == 0xFF && s[1] == 0xFE) return 2;                 // UTF-16LE BOM

    if (has_utf8_pattern(s, length)) return 3;

    return 1; // ANSI
}

// Unicode (wchar_t в Windows = UTF-16LE)
int getstringencoding(const wchar_t* str, int length)
{
    if (!str || length <= 0) return 0;
    return 2;
}

// Демонстрация
int main()
{
    setlocale(LC_ALL, "ru_RU.utf8");

    char a[128] = "one two three two";
    if (strreplace(a, "two", "TWO", 128))
        cout << "strreplace A: " << a << "\n";
    else
        cout << "strreplace A error, code=" << GetLastError() << "\n";

    wchar_t w[128] = L"cat dog cat";
    if (strreplace(w, L"cat", L"CAT", 128))
        wcout << L"strreplace W: " << w << L"\n";
    else
        wcout << L"strreplace W error, code=" << GetLastError() << L"\n";

    const char* csvA = "apple,\"green, pear\",plum";
    char fieldA[64];
    if (parsecsventry(csvA, 1, fieldA, 64))
        cout << "CSV A field 1: " << fieldA << "\n";

    const wchar_t* csvW = L"red,\"light blue\",green";
    wchar_t fieldW[64];
    if (parsecsventry(csvW, 2, fieldW, 64))
        wcout << L"CSV W field 2: " << fieldW << L"\n";

    const char* s1 = "Hello";
    const char* s2 = "Привет";

    cout << "Encoding s1: " << getstringencoding(s1, (int)strlen(s1)) << "\n";
    cout << "Encoding s2: " << getstringencoding(s2, (int)strlen(s2)) << "\n";

    wchar_t us[] = L"Unicode text";
    wcout << L"Encoding us: " << getstringencoding(us, (int)wcslen(us)) << L"\n";

    return 0;
}
