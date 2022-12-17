#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <filesystem>

//constexpr uint32_t HEADER_SIZE = sizeof(BNTXHeader) + sizeof(NXHeader);
constexpr uint32_t MEM_POOL_SIZE = 0x150;
// constexpr uint32_t DATA_PTR_SIZE = 0x8;

// constexpr uint32_t START_OF_STR_SECTION = HEADER_SIZE + MEM_POOL_SIZE + DATA_PTR_SIZE;

// constexpr uint32_t STR_HEADER_SIZE = 0x14;
// constexpr uint32_t EMPTY_STR_SIZE = 0x4;

// constexpr uint32_t FILENAME_STR_OFFSET = START_OF_STR_SECTION + STR_HEADER_SIZE + EMPTY_STR_SIZE;

// constexpr uint32_t BRTD_SECTION_START = 0xFF0;
// constexpr uint32_t START_OF_TEXTURE_DATA = BRTD_SECTION_START + 16;

// constexpr char DIC_SECTION[] = "\x5F\x44\x49\x43\x01\x00\x00\x00\xFF\xFF\xFF\xFF\x01\x00\x00\x00\xB4\x01\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x01\x00\xB8\x01\x00\x00\x00\x00\x00\x00";

#pragma pack(push, 1)

// Can be memory-mapped.
template<std::size_t Alignment> struct _BRTISection {
    char Magic[4] = { 'B', 'R', 'T', 'I' };
    uint32_t Size;
    uint64_t OffsetToNext;
    uint8_t  Flags;
    uint8_t  ImageStorageDimension;
    uint16_t TileMode;
    uint16_t Swizzle;
    uint16_t MipCount;
    uint16_t NumMultiSample;
    char Reserved[2] = { 0 };
    uint32_t Format;
    uint32_t GPUAccessFlags;
    uint32_t Width;
    uint32_t Height;
    uint32_t Depth;
    uint32_t ArrayLength;
    uint64_t BlockHeightLog2; // char TextureLayout[8];
    char Reserved2[20] = {0};

    uint32_t ImageSize;
    uint32_t Align = Alignment;
    uint32_t CompSel;
    uint8_t ImageDimension;
    char Reserved3[3] = {0};

    uint64_t NameOffset;
    uint64_t ParentOffset;

    uint64_t MipMapArrayPtr;
    uint64_t UserDataPtr;
    uint64_t TexturePtr;
    uint64_t TextureViewPtr;
    uint64_t UserDescriptorSlot;
    uint64_t UserDataDicPtr;
    char Reserved4[Alignment] = {0};
    uint64_t TextureDataPtr;
};

using BRTISection = _BRTISection<512>;

// Can be memory-mapped.
struct BRTDSection {
    char Magic[4] = { 'B', 'R', 'T', 'D' };
    uint32_t Padding;
    uint64_t ImageSize;
};

// Can be memory-mapped.
struct BNTXHeader {
    char Magic[8] = { 'B', 'N', 'T', 'X', 0, 0, 0, 0 };
    uint8_t VersionPatch;
    uint8_t VersionMinor;
    uint16_t VersionMajor;
    uint16_t BOM;
    uint8_t AlignmentShift;
    uint8_t TargetAddressSize;
    uint32_t FileNameOffset;
    uint16_t Flag;
    uint16_t STROffset;
    uint32_t RLTOffset;
    uint32_t FileSize;
};

// Can be memory-mapped.
struct NXHeader {
    char Magic[4] = { 'N', 'X', ' ', ' ' };
    uint32_t TextureCount;
    uint64_t BRTIArrayOffset;
    uint64_t BRTDOffset;
    uint64_t DICOffset;
    uint64_t DICSize;
};

// Can be memory-mapped.
struct RLTSection {
    uint64_t Offset;
    uint32_t Position;
    uint32_t Size;
    uint32_t Index;
    uint32_t Count;
};

// CANNOT be memory-mapped.
struct RLTTable {

    // Can be memory-mapped.
    struct RLTEntry {
        uint32_t Position;
        uint16_t StructCount;
        uint8_t  OffsetCount;
        uint8_t  PaddingCount;
    };

    char Magic[4] = { '_', 'R', 'L', 'T' };
    uint32_t RLTSectionPosition;
    uint32_t RLTSectionCount;
    std::vector<RLTSection> Sections;
    std::vector<RLTEntry> Entries;

    void Parse(std::istream& stream);

    void Write(std::ostream& stream);

    static RLTTable FromSectionSizes(const uint32_t StringPoolOffset, const uint32_t TextureDataOffset, const uint32_t ImageSize, const uint32_t str_section_size, const uint32_t dict_section_size);

    uint64_t GetSize();
};

// CANNOT be memory-mapped.
struct DICSection {

    struct DICEntry {
        int32_t ReferenceBit;
        uint16_t Children[2];
        uint64_t KeyStringOffset;
    };

    char Magic[4] = { '_', 'D', 'I', 'C' };
    int32_t Count;
    std::vector<DICEntry> Entries;

    static DICSection Default();

    void Parse(std::istream& stream);

    void Write(std::ostream& stream);

    uint64_t GetSize();
};

// CANNOT be memory-mapped.
struct STRSection {
    char Magic[4] = { '_', 'S', 'T', 'R' };
    uint32_t SizeOfSTRAndDIC;
    uint32_t SizeOfSTRAndDIC2;
    uint32_t unk3;
    uint32_t StringCount;
    uint32_t Reserved;
    std::vector<std::string> Strings;

    static STRSection FromStringsAndDICSection(const std::vector<std::string>& input, DICSection& dic_section);

    void Parse(std::istream& stream);

    void Write(std::ostream& stream);

    uint64_t GetOffset(const std::string& str);

    uint64_t GetSize();
};

#pragma pack(pop)

class BNTX {
private:
    BNTXHeader FileHeader;
    NXHeader PlatformHeader;
    char MemPool[MEM_POOL_SIZE] = { 0 };
    std::vector<uint64_t> TextureInfoOffsets;

    STRSection StringPool;
    DICSection Dictionary;
    std::vector<BRTISection> TextureInfos;
    BRTDSection TextureData;

    std::vector<std::vector<uint8_t>> Images;

    RLTTable RelocationTable;
public:
    BNTX(cv::Mat& mat, const std::string& name);

    BNTX(const std::filesystem::path& path);

    void Write(const std::string& path);
};