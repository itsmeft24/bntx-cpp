#pragma once

#include <fstream>
#include <DirectXTex.h>

#include "BNTX.hpp"
#include "tegra_swizzle.hpp"

void RLTTable::Parse(std::istream& stream) {
	stream.read(reinterpret_cast<char*>(this), 12);
	stream.seekg(4, std::ios_base::cur);
	Sections.resize(RLTSectionCount);
	stream.read(reinterpret_cast<char*>(Sections.data()), RLTSectionCount * sizeof(RLTSection));

	int entry_count = 0;
	for (const auto& section : Sections) {
		entry_count += section.Count;
	}

	Entries.resize(entry_count);
	stream.read(reinterpret_cast<char*>(Entries.data()), entry_count * sizeof(RLTEntry));
}

void RLTTable::Write(std::ostream& stream) {
	stream.write(reinterpret_cast<char*>(this), 12);
	stream.seekp(4, std::ios_base::cur);
	stream.write(reinterpret_cast<char*>(Sections.data()), Sections.size() * sizeof(RLTSection));
	stream.write(reinterpret_cast<char*>(Entries.data()), Entries.size() * sizeof(RLTEntry));
}

RLTTable RLTTable::FromSectionSizes(const uint32_t StringPoolOffset, const uint32_t TextureDataOffset, const uint32_t ImageSize, const uint32_t str_section_size, const uint32_t dict_section_size) {

	return {
		{ '_', 'R', 'L', 'T' },
		TextureDataOffset + (uint32_t)sizeof(BRTDSection) + (uint32_t)ImageSize,
		2,
		{
			RLTSection { 0, 0, StringPoolOffset + str_section_size + dict_section_size + (uint32_t)sizeof(BRTISection), 0, 4 },
			RLTSection { 0, TextureDataOffset, (uint32_t)sizeof(BRTDSection) + (uint32_t)ImageSize, 4, 1 }
		},
		{
			RLTEntry { sizeof(BNTXHeader) + 0x8, 2, 1, (sizeof(NXHeader) + MEM_POOL_SIZE - 0x10) / 8 },
			RLTEntry { sizeof(BNTXHeader) + 0x18, 2, 2, (uint8_t)((StringPoolOffset + str_section_size + dict_section_size + 0x80 - (sizeof(BNTXHeader) + sizeof(NXHeader))) / 8) },
			RLTEntry { StringPoolOffset + str_section_size + 0x10, 2, 1, 1 },
			RLTEntry { StringPoolOffset + str_section_size + dict_section_size + 0x60, 1, 3, 0 },
			RLTEntry { sizeof(BNTXHeader) + 0x10, 2, 1, (uint8_t)((StringPoolOffset + str_section_size + dict_section_size + sizeof(BRTISection) - 0x8 - (sizeof(BNTXHeader) + 0x18)) / 8) }
		}
	};
}

uint64_t RLTTable::GetSize() {
	return 0x10 + sizeof(RLTSection) * Sections.size() + sizeof(RLTEntry) * Entries.size();
}

DICSection DICSection::Default() {
	return {
		{ '_', 'D', 'I', 'C' },
		1,
		{
			{
				-1,
				{
					1,
					0
				},
				// Points to an empty string.
				436
			},
			{
				1,
				{
					0,
					1
				},
				// Points to an image name. Can be equal to, but is not always the same as the file name.
				// In this case, since we default to one image, it is just the file name.
				440
			},
		}
	};
}

void DICSection::Parse(std::istream& stream) {
	stream.read(reinterpret_cast<char*>(this), 8);
	this->Entries.resize(this->Count + 1);
	stream.read(reinterpret_cast<char*>(this->Entries.data()), Entries.size() * sizeof(DICEntry));
}

void DICSection::Write(std::ostream& stream) {
	this->Count = this->Entries.size() - 1;
	stream.write(reinterpret_cast<char*>(this), 8);
	stream.write(reinterpret_cast<char*>(this->Entries.data()), Entries.size() * sizeof(DICEntry));
}

uint64_t DICSection::GetSize() {
	return 8 + this->Entries.size() * sizeof(DICEntry);
}

STRSection STRSection::FromStringsAndDICSection(const std::vector<std::string>& input, DICSection& dic_section) {
	STRSection ret = {
		{ '_', 'S', 'T', 'R' },
		0xCC,
		0xCC,
		0,
		input.size(),
		0,
		input
	};
	ret.SizeOfSTRAndDIC = (uint32_t)ret.GetSize() + dic_section.GetSize();
	ret.SizeOfSTRAndDIC2 = (uint32_t)ret.GetSize() + dic_section.GetSize();
	return ret;
}

void STRSection::Parse(std::istream& stream) {
	stream.read(reinterpret_cast<char*>(this), 24);

	for (int x = 0; x < StringCount; x++) {
		uint16_t size = 0;
		stream.read(reinterpret_cast<char*>(&size), 2);
		char* tmp = new char[size]();
		stream.read(tmp, size);

		// Align to the next two bytes.
		if (size % 2 == 0) {
			stream.seekg(2, std::ios_base::cur);
		}
		else {
			stream.seekg(1, std::ios_base::cur);
		}

		Strings.push_back(std::string(tmp, size));		
		delete[] tmp;
	}
}

void STRSection::Write(std::ostream& stream) {
	stream.write(reinterpret_cast<char*>(this), 24);

	for (const auto& str : Strings) {
		uint16_t size = str.size();
		stream.write(reinterpret_cast<char*>(&size), 2);
		stream.write(str.c_str(), str.size());

		// Align to the next two bytes.
		if (size % 2 == 0) {
			stream.seekp(2, std::ios_base::cur);
		}
		else {
			stream.seekp(1, std::ios_base::cur);
		}
	}

	// Align to the next 8 bytes.
	stream.seekp(0x8 - (stream.tellp() % 0x8), std::ios_base::cur);
}

uint64_t STRSection::GetOffset(const std::string& input)
{
	uint64_t ret = 24;
	for (const auto& str : Strings) {
		if (str == input) {
			break;
		}
		ret += str.size() + 3; // 2 bytes of length, 1 of extra space
	}
	return ret;
}

uint64_t STRSection::GetSize() {
	uint64_t ret = 24;
	for (const auto& str : Strings)
		ret += str.size() + 3; // 2 bytes of length, 1 of extra space
	ret = (ret / 0x8 + 1) * 0x8;
	// ret = round_up(ret, 8);
	return ret;
}

BNTX::BNTX(cv::Mat& mat, const std::string& name) {

	
	DirectX::Image dximg{
		mat.cols,
		mat.rows,
		DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		mat.cols * 4,
		mat.cols * mat.rows * 4,
		mat.data
	};

	DirectX::ScratchImage SImg;

	std::cout << "[SteveModMaker::BNTX::BNTX] Compressing image to BC1_UNORM..." << std::endl;

	// Should be similar to using Fast compression mode in Switch Toolbox.
	DirectX::Compress(dximg, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, SImg);

	std::cout << "[SteveModMaker::BNTX::BNTX] Swizzling image via tegra_swizzle..." << std::endl;

	uint64_t ImageSize = swizzled_mip_size(
		div_round_up(mat.cols, 4),
		div_round_up(mat.rows, 4),
		1,
		block_height_mip0(div_round_up(mat.rows, 4)),
		8 // 8 for BC1, 16 for BC7, 4 for RGBA8
	);

	std::vector<uint8_t> Image(ImageSize);

	swizzle_block_linear(
		div_round_up(mat.cols, 4),
		div_round_up(mat.rows, 4),
		1,
		SImg.GetPixels(),
		SImg.GetPixelsSize(),
		Image.data(),
		ImageSize,
		block_height_mip0(div_round_up(mat.rows, 4)),
		8 // 8 for BC1, 16 for BC7, 4 for RGBA8
	);

	Images.push_back(std::move(Image));
	
	/*
	uint64_t ImageSize = swizzled_mip_size(
		mat.cols,
		mat.rows,
		1,
		block_height_mip0(mat.rows),
		4
	);

	IMAGE_DATA.resize(ImageSize);

	swizzle_block_linear(
		mat.cols,
		mat.rows,
		1,
		mat.data,
		mat.cols * mat.rows * 4,
		IMAGE_DATA.data(),
		ImageSize,
		block_height_mip0(mat.rows),
		4
	);
	*/

	Dictionary = DICSection::Default();

	StringPool = STRSection::FromStringsAndDICSection({ name }, Dictionary);

	// There is only one BRTIOffset so just adding 8 bytes to the header size is fine.
	uint64_t StringPoolOffset = sizeof(BNTXHeader) + sizeof(NXHeader) + sizeof(MemPool) + sizeof(uint64_t);
	uint64_t TextureInfoOffset = StringPoolOffset + StringPool.GetSize() + Dictionary.GetSize();

	TextureInfoOffsets.push_back(TextureInfoOffset);

	uint32_t TextureDataOffset = StringPoolOffset + StringPool.GetSize() + Dictionary.GetSize() + sizeof(BRTISection);

	RelocationTable = RLTTable::FromSectionSizes(StringPoolOffset, TextureDataOffset, ImageSize, StringPool.GetSize(), Dictionary.GetSize());

	TextureInfos = {
		{
			.Magic = { 'B', 'R', 'T', 'I' },
			.Size = uint32_t(labs(TextureDataOffset - TextureInfoOffset)),
			.OffsetToNext = uint32_t(labs(TextureDataOffset - TextureInfoOffset)),
			.Flags = 1,
			.ImageStorageDimension = 2,
			.TileMode = 0,
			.Swizzle = 0,
			.MipCount = 1,
			.NumMultiSample = 1,
			.Reserved = {0,0},
			.Format = 0x1A01, // BC1_UNORM is 0x1A01, BC7_UNORM is 0x2001, R8G8B8A8_UNORM_SRGB is 0x0B06, 0x0C06 is B8G8R8A8_UNORM_SRGB
			.GPUAccessFlags = 32,
			.Width = uint32_t(mat.cols),
			.Height = uint32_t(mat.rows),
			.Depth = 1,
			.ArrayLength = 1,
			.BlockHeightLog2 = std::bit_width(block_height_mip0(div_round_up(mat.rows, 4))) - 1,
			// Calculate using log2(block_height).
			// .Reserved2 = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
			.ImageSize = uint32_t(ImageSize),
			// .Align = 512,
			.CompSel = 84148994,
			.ImageDimension = 1,
			// .Reserved3 = {0,0,0},
			.NameOffset = StringPoolOffset + StringPool.GetOffset(name),
			.ParentOffset = 32,
			.MipMapArrayPtr = StringPoolOffset + StringPool.GetSize() + Dictionary.GetSize() + sizeof(BRTISection) - 8,
			.UserDataPtr = 0,
			.TexturePtr = StringPoolOffset + StringPool.GetSize() + Dictionary.GetSize() + sizeof(BRTISection) - 0x208,
			.TextureViewPtr = StringPoolOffset + StringPool.GetSize() + Dictionary.GetSize() + sizeof(BRTISection) - 0x108,
			.UserDescriptorSlot = 0,
			.UserDataDicPtr = 0,
			// .Reserved4 = {0},
			.TextureDataPtr = TextureDataOffset + sizeof(BRTDSection)
		}
	};

	TextureData =
	{
		{ 'B', 'R', 'T', 'D'},
		0,
		ImageSize + sizeof(BRTDSection)
	};

	FileHeader = {
		{ 'B', 'N', 'T', 'X', 0, 0, 0, 0 },
		0,
		0,
		4,
		0xFEFF,
		0x40,
		0x0C,
		static_cast<uint32_t>(StringPoolOffset + StringPool.GetOffset(name) + 0x2),
		0,
		(uint16_t)StringPoolOffset,
		TextureDataOffset + (uint32_t)sizeof(BRTDSection) + (uint32_t)ImageSize,
		TextureDataOffset + (uint32_t)sizeof(BRTDSection) + (uint32_t)ImageSize + (uint32_t)RelocationTable.GetSize(), // size of file
	};

	PlatformHeader = {
		{ 'N', 'X', ' ', ' ' },
		1,
		StringPoolOffset - TextureInfoOffsets.size() * sizeof(uint64_t),
		TextureDataOffset,
		StringPoolOffset + StringPool.GetSize(),
		StringPool.GetSize() + Dictionary.GetSize(),
	};

}

BNTX::BNTX(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::in | std::ios::binary);
	file.read(reinterpret_cast<char*>(&FileHeader), sizeof(BNTXHeader));
	file.read(reinterpret_cast<char*>(&PlatformHeader), sizeof(NXHeader));
	file.seekg(MEM_POOL_SIZE, std::ios_base::cur);
	
	TextureInfoOffsets.resize(PlatformHeader.TextureCount);
	file.read(reinterpret_cast<char*>(TextureInfoOffsets.data()), TextureInfoOffsets.size() * sizeof(uint64_t));

	StringPool.Parse(file);

	file.seekg(PlatformHeader.DICOffset);

	Dictionary.Parse(file);

	TextureInfos.reserve(PlatformHeader.TextureCount);

	std::streamoff image_base = PlatformHeader.BRTDOffset + sizeof(BRTDSection);

	for (int x = 0; x < PlatformHeader.TextureCount; x++) {
		BRTISection brti_section{};
		file.read(reinterpret_cast<char*>(&brti_section), sizeof(BRTISection));
		TextureInfos.push_back(brti_section);
		std::streampos current_offset = file.tellg();

		file.seekg(image_base);
		std::vector<uint8_t> bytes(brti_section.ImageSize);
		file.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
		Images.push_back(std::move(bytes));
		image_base += brti_section.ImageSize;

		file.seekg(current_offset + std::streamoff(brti_section.OffsetToNext - sizeof(BRTISection)));
	}

	file.read(reinterpret_cast<char*>(&TextureData), sizeof(BRTDSection));
	file.seekg(FileHeader.RLTOffset, std::ios_base::beg);
	RelocationTable.Parse(file);
	file.close();
}

void BNTX::Write(const std::string& path) {
	std::cout << "[SteveModMaker::BNTX::Write] Writing image to BNTX..." << std::endl;
	std::ofstream file(path, std::ios::out | std::ios::binary);
	file.write(reinterpret_cast<char*>(&FileHeader), sizeof(BNTXHeader));
	file.write(reinterpret_cast<char*>(&PlatformHeader), sizeof(NXHeader));
	file.write(MemPool, MEM_POOL_SIZE);
	file.write(reinterpret_cast<char*>(TextureInfoOffsets.data()), TextureInfoOffsets.size() * sizeof(uint64_t));
	StringPool.Write(file);
	Dictionary.Write(file);
	for (const auto& texture_info : TextureInfos) {
		file.write(reinterpret_cast<const char*>(&texture_info), sizeof(BRTISection));
	}

	uint64_t START_OF_STR_SECTION = sizeof(BNTXHeader) + sizeof(NXHeader) + sizeof(MemPool) + TextureInfoOffsets.size() * sizeof(uint64_t);
	file.seekp(START_OF_STR_SECTION + StringPool.GetSize() + Dictionary.GetSize() + sizeof(BRTISection));
	file.seekp(PlatformHeader.BRTDOffset);
	file.write(reinterpret_cast<char*>(&TextureData), sizeof(BRTDSection));
	for (const auto& image : Images) {
		file.write(reinterpret_cast<const char*>(image.data()), image.size());
	}
	file.seekp(FileHeader.RLTOffset);
	RelocationTable.Write(file);
	file.close();
}